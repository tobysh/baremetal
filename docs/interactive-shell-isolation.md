# Interactive shell: isolation design

Status: design, not implemented (30 October 2026). Decision: one microVM per shell session. Threat model and the existing one-shot sandbox: `docs/security.md`. Lab content: `docs/security-labs.md`.

## Why not bubblewrap

The one-shot runner works because jobs are short, read-only, offline and narrow (compiler plus the learner's binary), so a seccomp denylist is tolerable. A shell session is long, writable, has the full userland and often needs a network. The only barrier between a hostile learner and the host kernel would be a denylist. The shell therefore runs behind a hardware virtualisation boundary; the existing sandbox stays for lesson runs.

## Trust boundaries

```
browser --WS--> web app (Next.js, MongoDB) --Unix socket--> shell broker (own uid, no DB access)
                                                              |
                                                     jailer + Firecracker  (one per session)
                                                              |
                                                       guest kernel + rootfs
```

- The web app authenticates, applies quotas, and asks the broker for a session. It never sees guest traffic beyond the byte stream.
- The broker is a separate service like the runner (dedicated uid, systemd hardening, socket restricted to the web group). It holds no `AUTH_SECRET`, no DB credentials, no API keys.
- The guest is assumed fully hostile and holds no secrets. Anything inside it is learner-readable.

## Per-session VM

- Firecracker started through `jailer` (chroot, own uid/gid per VM, cgroup, seccomp on the VMM, new pid/net namespaces). `/dev/kvm` access goes to the jailer uid only; do not leave it mode 0666 (it is on the current dev machine).
- Guest kernel: minimal config, no modules, no unneeded drivers, `init=` a small agent. Rootfs is a shared read-only image (squashfs/erofs) with a per-session tmpfs or small sparse overlay, destroyed on exit. No persistent disk by default; lab progress is stored by the web app, not the guest.
- Limits: 1 vCPU, 256-512 MiB RAM, overlay 64-128 MiB, CPU rate limited by the VMM and the jailer cgroup, hard wall-clock cap (for example 30 min absolute, 5 min idle), output and keystroke-rate caps on the stream.
- Start from a snapshot of a booted guest for sub-second startup. Each restored snapshot must reseed its RNG and generate its own per-session flags; never reuse a snapshot that holds session secrets.
- The guest talks to the broker only over vsock, using one fixed protocol (PTY bytes, resize, exit). The agent in the guest runs as root inside the VM; learners get a normal user plus `sudo` only where a lab needs it. The broker treats everything from vsock as untrusted data, with bounded frame sizes.

## Network

Default is no NIC. Labs that need a target get a NIC on a per-session tap in its own network namespace:

- The only reachable peer is that session's target (a second small VM or a process in the same guest). Prefer a process on loopback in the same guest where the lesson allows: no NIC at all.
- Nftables default-drop in the namespace: no host address, no other sessions, no link-local/metadata ranges, no RFC1918, no internet. Test each of these from inside a guest.
- Rate limit packets per tap; drop broadcast/multicast.

## Admission and abuse control

- Verified accounts only; one concurrent session per account, small global cap sized from host RAM (sessions are far heavier than runs).
- Quotas per account per day; a session counts against budget even if it dies early.
- Broker-side detection: sustained 100% CPU, packet-rate limits, fork storms inside the guest (guest pids cgroup), all end the session. Kill is by destroying the VM, not by asking the agent.
- Audit log (session id, account hash, start/end, reason for termination), no keystroke content stored.
- Terms/Privacy: update `content/legal/` and `SITE.legal.lastUpdated` if any session data is stored.

## Host

- Dedicated host or VM for the broker with nothing else on it: no MongoDB, no web app, no secrets. Keep the host kernel and Firecracker patched; subscribe to Firecracker and KVM advisories.
- Nested virtualisation on cloud VMs is slower and weaker; prefer bare metal or a provider with KVM exposed.
- Mitigations for CPU side channels depend on the host CPU; keep SMT policy and `mitigations=` at secure defaults, and do not co-schedule sessions of different learners on one core if the host is shared with anything sensitive.

## Verification (before launch)

On a disposable host, with a hostile test image:

1. Fork bomb, memory bomb, disk fill, and busy loop: session ends, host unaffected.
2. From inside the guest: no route to host, other sessions, internet, link-local, DNS; vsock only reaches the broker.
3. Guest cannot open `/dev/kvm`, cannot see host processes, files or env.
4. Broker fuzzing: oversized/malformed vsock frames, slow readers, hung agents.
5. Teardown: killing the web app, the broker or the VM leaves no orphan VMs, taps, namespaces or overlay files (reaper on boot).
6. A kernel/hypervisor-level escape is out of scope for tests; it is the residual risk this design accepts, mitigated by an empty host and patching.

## Implementation order

1. Prototype launcher script (`scripts/shell/`): boot one microVM from a rootfs, attach a PTY over vsock, enforce timeouts. Run only on a disposable VM.
2. Broker service plus protocol, systemd units under `deploy/systemd/`, tests in `scripts/shell/`.
3. Web endpoint (WebSocket), terminal UI (lazy-loaded), quotas.
4. Lab images and per-session flags.
5. Verification above, then docs/security.md update.

## Open questions

- Hosting: bare metal or KVM-capable provider?
- Do any labs need two machines, or is a loopback target enough?
- Persist learner files between sessions? (Default no.)

## Prototype findings (30 October 2026)

`scripts/shell/` (`build-rootfs.sh`, `launch.sh`, `launch-jailed.sh`) boots one throwaway VM on the serial console. Verified on a development machine, benign commands only: Firecracker 1.17.0, guest kernel 6.1.141 (Firecracker CI build, no modules, vsock and ext4 built in), jailer with cgroup v2.

Verified:
- Guest shell runs as uid 1000, no groups, no capabilities, `no_new_privs`; root filesystem read-only; `mount` refused; no NIC.
- Unjailed and jailed boots reach a shell in about a second; a hung guest is killed at the wall-clock cap; nothing is left behind (processes, chroot, cgroup) after a normal exit or a cap kill.

Not verified: hostile workloads (fork/memory bombs, escape attempts), the jailer's seccomp and uid isolation under attack, any network setup, vsock. These need a disposable machine.

Lessons that shape the broker:
1. **Use the official static release binaries** (checksum-verified). The distro build is dynamically linked and cannot exec in the empty jailer chroot ("No such file or directory").
2. **The jailer names its chroot and cgroup after the exec file's name** (`<base>/<exec-file-name>/<id>/root`), not always `firecracker`. Derive paths from it.
3. **`--new-pid-ns` makes the jailer's parent exit immediately.** Its exit is not the end of the VM. Track the VM by its cgroup (`cgroup.procs`), and end it with `cgroup.kill`. cgroup files report size 0, so test contents, not `-s`.
4. **The guest cannot end the VM.** Sysrq poweroff/reboot do not stop Firecracker; only init exiting (kernel panic with `panic=1`, `reboot=k`) does. The broker must own VM lifetime and kill on session end, timeout or disconnect.
5. **Serial input sent before the shell is ready is dropped.** The broker should forward input only after the guest agent signals ready.
6. **The jailer leaves the chroot and cgroup behind.** The broker needs cleanup on exit and a reaper at startup.
7. **The CI kernel is a binary fetched over HTTP.** Build and pin your own kernel with a verified hash for production.
8. `/dev/kvm` should be limited to the jailer's uid in production (0666 on the dev machine).

## Broker (implemented, development-tested)

Code: `src/lib/shell/` (`frames.ts` codec, `vm.ts` VM drivers, `broker.ts` sessions), `scripts/shell-broker.mts` (service entry), `scripts/shell/agent.c` (guest agent, built statically into the rootfs by `build-rootfs.sh`), `scripts/shell/broker-checks.mts`.

How it works: the web side connects to the broker's Unix socket (mode 0660; set the group in the unit), sends an `OPEN` frame `{account, cols, rows}` (account is an opaque keyed hash chosen by the web app; the broker never sees identities) and waits for `OPENED`. The broker starts a VM, connects to the guest agent through Firecracker's vsock proxy, and waits for the agent's `READY` before forwarding input. Frames are `[type u8][len u32][payload ≤ 16 KiB]` on both hops, and each parser enforces the bound itself because the guest is hostile.

Enforced by the broker, never by the guest: one shell per account (checked before the global cap so the message is specific), a global session cap (booting sessions count), boot timeout, idle timeout (keystrokes and resizes only), absolute cap from admission, a token bucket on input, a total output cap, backpressure in both directions, and strict frame validation (any unexpected frame type ends the session). Every session ends through one path: the VM is destroyed first, and capacity and the account lock are released only afterwards. A client that leaves while the VM is still starting does not leak it. Audit lines carry ids, counters and the end reason, never terminal content.

Run the checks: `SHELL_KERNEL=... SHELL_ROOTFS=... FIRECRACKER=/usr/bin/firecracker npx tsx scripts/shell/broker-checks.mts` (fake-guest checks run without the variables; the real-VM checks boot about ten microVMs, benign commands only). Last run: all pass, shell ready in about 0.6 s.

Not done, in priority order:
1. **Jailed driver is untested.** `startJailedVm` mirrors the verified `launch-jailed.sh` but has not run. It needs a run as root on a disposable machine (`SHELL_MODE=jailed`, static release binaries).
2. **Privilege separation.** Starting the jailer needs root, so today the broker would run as root. Split a tiny root launcher (create/kill VM by id over a private socket, no network, no learner data) from the unprivileged broker that parses guest traffic. Until then, do not run the broker as root on a host that holds anything else.
3. **Web endpoint and terminal UI** (WebSocket to the broker, account hashing, verified-account gate, daily quotas, `content/legal/` update and `SITE.legal.lastUpdated` if anything is stored).
4. **systemd units**, kernel pinned and built by you, lab images and per-session flags, networking for labs.
5. **Hostile-workload tests** on a disposable machine (fork/memory bombs, disk fill, network egress, escape attempts against the jailer).
6. Output rate limiting beyond the total cap (a guest can currently send up to the cap as fast as the web side reads it).
