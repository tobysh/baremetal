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
