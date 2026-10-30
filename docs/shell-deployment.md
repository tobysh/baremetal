# Interactive shell: host deployment

Status: written 30 October 2026 and **not yet exercised on a real host**. Everything marked UNVERIFIED below is derived from documentation or from the prototype, not from a boot under these units. Design and threat model: `docs/interactive-shell-isolation.md`. General web/runner deployment: `docs/deployment-hardening.md`. Do the checklist at the end before any learner can reach a shell.

## What gets installed

| Artefact | Source in the repository | Installed at |
|---|---|---|
| Slice (limits for everything below) | `deploy/systemd/baremetal-shell.slice` | `/etc/systemd/system/` |
| Launcher (root, starts and kills VMs only) | `deploy/systemd/baremetal-shell-launcher.service`, entry `scripts/shell-launcher.mts` (being written) | `/etc/systemd/system/` |
| Broker (unprivileged, sessions and limits) | `deploy/systemd/baremetal-shell-broker.service`, entry `scripts/shell-broker.mts` | `/etc/systemd/system/` |
| Socket directories | `deploy/shell/tmpfiles.d/baremetal-shell.conf` | `/etc/tmpfiles.d/` |
| `/dev/kvm` permissions | `deploy/shell/udev/99-baremetal-kvm.rules` | `/etc/udev/rules.d/` |
| Firecracker 1.17.0 + jailer (static) | downloaded by `scripts/shell/install-host.sh`, sha256 pinned | `/usr/local/libexec/baremetal-shell/v1.17.0/`, symlink `current` |
| Guest kernel (6.1.191) | `scripts/shell/build-kernel.sh` | `/usr/local/share/baremetal-shell/images/vmlinux` |
| Guest rootfs | `scripts/shell/build-rootfs.sh` | `/usr/local/share/baremetal-shell/images/rootfs.ext4` |
| Host firewall | `deploy/shell/nftables/host-egress.nft` | `/etc/nftables.conf` or an include |
| Lab namespace firewall | `deploy/shell/nftables/lab-session.nft` (tested by `test-lab-session.sh`) | applied per session by the launcher |
| Code | the repository | `/opt/baremetal-shell` (root-owned, read-only to services) |

Users and groups (all created by `install-host.sh`): user `baremetal-shell-broker` (no login, primary group `baremetal-shell-client`, supplementary `baremetal-shell-launch`). The launcher runs as root with a reduced capability set; there is deliberately no launcher user. Group `baremetal-shell-client` is what the **web service** must join (`SupplementaryGroups=baremetal-shell-client` in `baremetal-web.service`, not changed by this work). Group `baremetal-shell-launch` is the only group that can connect to the launcher socket, and only the broker is in it. Firecracker processes run under numeric uids 300000-365535 with no accounts behind them; the script refuses to continue if any account sits in that range. The prototype uses a single uid (`SHELL_VM_UID`, 300001) for every VM. One uid per session is the stronger setup (TODO-to-confirm with the launcher author; if adopted, the unit needs no change because the range is already reserved).

## Host requirements

- **Bare metal, or a provider that exposes KVM to your instance** (`/dev/kvm` present, `vmx`/`svm` in `/proc/cpuinfo`). `install-host.sh` fails on a virtualised host unless `--allow-virtualized` is given. Nested virtualisation is slower, depends on the outer hypervisor's correctness, and puts a second party between you and the CPU's isolation features. Acceptable for staging only.
- **Dedicated machine.** Nothing else on it: no MongoDB, no web app, no secrets, no other tenants. The units hide `/etc/baremetal`, `/var/lib/baremetal`, `/opt/baremetal-web` and `/opt/baremetal-runner` as a second line, not a licence to share the host. **Conflict to resolve before launch:** the web app reaches the broker over a Unix socket, which only works on the same machine. A truly separate shell host needs a transport (for example an authenticated tunnel terminating in a local socket) that nobody has designed or reviewed yet. Until then the choice is "same host, hardened" or "new design".
- **CPU and kernel.** Current host kernel LTS with the vendor's microcode. Do not boot with `mitigations=off` (the script fails on it). `install-host.sh` prints `/sys/devices/system/cpu/vulnerabilities/*` for the record; read it, and compare it with Firecracker's production host setup guide for your CPU model.
- **SMT.** Guests run with `smt: false` in the VM config, which hides siblings from the guest but does not stop the host scheduler from placing another VM's vCPU on the sibling thread. With sessions of different learners on one host, choose one: (a) `nosmt` on the kernel command line (simplest, costs capacity), (b) core-scheduling / one-VM-per-core pinning (needs launcher support, not implemented), (c) accept the cross-learner side-channel risk in writing. Default recommendation: (a).
- **Memory.** No host swap (`swapoff -a`, remove from fstab): guest memory in swap outlives the VM. KSM off (`/sys/kernel/mm/ksm/run` = 0). The slice sets `MemorySwapMax=0` as well.
- **Filesystem.** `/var/lib` (the jail base) must not be mounted `nodev` because the jailer calls `mknod` for `/dev/kvm` inside every chroot; `install-host.sh` checks this. Prefer a filesystem with reflinks (XFS, btrfs) so the per-session copy of the 128 MiB rootfs is free; on ext4 it is a real copy.
- **systemd 257 or newer** (`ProtectControlGroups=private`), cgroup v2 only.
- **Clock.** Guests take time from the host. Keep NTP working (the firewall allows it).

## Install order

1. Provision the dedicated host. Check firmware: VT-x/AMD-V on, SMT policy decided.
2. `scripts/shell/install-host.sh --dry-run` and read it. Then as root: `scripts/shell/install-host.sh`. It checks the host, installs Firecracker, creates users, groups and directories, installs the units, the slice, the tmpfiles entry and the udev rule, and reloads systemd and udev. It enables and starts nothing. Safe to re-run.
3. Copy the repository to `/opt/baremetal-shell` (root-owned, mode 755 directories, no write for services) and `npm ci --omit=dev` there. The units run `node --import tsx` from this directory.
4. Build the guest kernel on a **separate build machine** (`scripts/shell/build-kernel.sh`, tens of minutes) and copy `vmlinux` over; verify against its `SHA256SUMS`. Build the rootfs with `scripts/shell/build-rootfs.sh` (it needs gcc and the host binaries it lists) and install it. Both files are root-owned, mode 0644.
5. Load the host firewall (below) with its addresses filled in. Test it from a second session before closing your SSH session.
6. Apply the boot-time `nosmt` decision and reboot if needed.
7. Start the launcher alone and run the first-start checks (next section). Then the broker. Then run `scripts/shell/hostile/` (pre-launch checklist). Only then add the web endpoint.

### First-start checks for the units (UNVERIFIED)

The units were checked with `systemd-analyze verify` and scored with `systemd-analyze security --offline`, which proves the directives exist and parse, not that Firecracker boots under them. Expect to adjust on the first start:

- `journalctl -u baremetal-shell-launcher -b` for `EPERM`, `Operation not permitted` or `Bad system call`. The capability list in `CapabilityBoundingSet=` and the `SystemCallFilter=` lines are a best derivation from the jailer's behaviour (chroot, pivot_root, mknod, chown, setuid, namespace creation). Add only the one item that the journal names. `SystemCallErrorNumber=EPERM` makes missing syscalls fail visibly instead of killing the process with SIGSYS.
- `MemoryDenyWriteExecute=yes` with `node --jitless`: Node without the JIT works under it in principle; WebAssembly would not. MDWE is inherited by the jailer and Firecracker. If Firecracker or the Node runtime fails to start because of it, remove it from the launcher first and record why.
- `Delegate=` + `DelegateSubgroup=supervisor` + `ProtectControlGroups=private`: the intent is that the jailer's default cgroup path (`/sys/fs/cgroup/firecracker/<id>`) lands inside the launcher unit's cgroup, so the slice limits cover every VM. Confirm after the first VM boots: `systemd-cgls /baremetal-shell.slice` must show the VM's cgroup under the launcher unit, and `cat /sys/fs/cgroup/baremetal-shell.slice/memory.max` must read 8G. If the private cgroup root turns out to be the `supervisor` subgroup (not the unit), the jailer cannot enable controllers there (Node process in the same cgroup); the fallback is to pass the jailer `--parent-cgroup` and make the cgroup root a launcher setting (TODO-to-confirm; `startJailedVm` already has a `cgroupRoot` option).
- `PrivateDevices=yes` with `DeviceAllow=/dev/kvm rwm`: the second makes the unit's device cgroup allow the node that the jailer creates inside each chroot. If a VM fails with "permission denied" on `/dev/kvm`, this is the place to look.
- `ProtectKernelTunables=yes` makes `/sys` read-only; check that the private cgroup mount stays writable.
- How the broker reaches a VM's vsock socket (`v.sock` inside the root-only chroot) is not decided in the units: the broker unit makes `/var/lib/baremetal-shell` inaccessible. The launcher protocol in `src/lib/shell/launcher.ts` returns a socket path "reachable by the broker's group"; the mechanism (a socket link in `/run/baremetal-shell`, or fd passing) is TODO-to-confirm, and the unit's `ReadOnlyPaths=`/`ReadWritePaths=` must follow it.

## Environment variables (TODO-to-confirm)

Confirmed from `scripts/shell-broker.mts` as of this writing: `SHELL_BROKER_SOCKET`, `SHELL_MAX_SESSIONS` (default 4), `SHELL_IDLE_SECONDS` (300), `SHELL_MAX_SECONDS` (1800); and, for the pre-split in-process root mode, `FIRECRACKER`, `JAILER`, `SHELL_KERNEL`, `SHELL_ROOTFS`, `SHELL_CHROOT_BASE`, `SHELL_VM_UID`, `SHELL_MODE`. After the privilege split these must be re-read from `scripts/shell-launcher.mts` and the new `scripts/shell-broker.mts`:

| Variable in the units | Where | Status |
|---|---|---|
| `SHELL_LAUNCHER_SOCKET=/run/baremetal-shell/launcher.sock` | both | name invented to make the units consistent; TODO-to-confirm |
| `FIRECRACKER`, `JAILER`, `SHELL_KERNEL`, `SHELL_ROOTFS`, `SHELL_CHROOT_BASE`, `SHELL_VM_UID` | launcher | names from the old broker; TODO-to-confirm |
| launcher VM cap (the launcher has `maxVms`, default 4) | launcher | no variable known; **must be at least `SHELL_MAX_SESSIONS`** or sessions are refused as `busy`. TODO-to-confirm |
| per-VM limits (jailer `cpu.max`, `memory.max`, `pids.max`: today 50% / 400M / 64) | launcher | hard-coded defaults in `startJailedVm` unless exposed; the slice sizing below assumes these |
| `SHELL_MODE` | broker | must not be settable in production (the unit does not set it) |
| web service: broker socket path | web | TODO-to-confirm; the web unit needs the same value as `SHELL_BROKER_SOCKET` |

`/etc/baremetal-shell/launcher.env` and `broker.env` are optional (`EnvironmentFile=-`) for overrides. They hold no secrets: the broker has none by design.

## Resource sizing

Let `S` be `SHELL_MAX_SESSIONS`. With the per-VM limits above (1 vCPU, 256 MiB guest, jailer cgroup `memory.max` 400 MiB, `cpu.max` 50%, `pids.max` 64):

    slice MemoryMax  = S x 400 MiB + 1 GiB             (launcher, broker, page-cache slack)
    slice CPUQuota   = S x 50%
    slice TasksMax   = S x 64 + 256
    host RAM         >= slice MemoryMax + 4 GiB        (OS, firewall, journal, build-free)
    host cores       >= S x 0.5 + 2                    (boots are bursts of about one core for 0.6 s; keep two cores for the host)
    jail disk        =  S x 128 MiB (no reflink) + 64 MiB; ~0 with reflinks
    launcher NOFILE  >= S x 16 + 256

| S | MemoryMax | CPUQuota | TasksMax | Minimum host |
|---|---|---|---|---|
| 4 (development default) | 3G | 200% | 512 | 4 cores, 8 GiB |
| 8 | 5G | 400% | 768 | 8 cores, 10 GiB |
| 16 (shipped slice) | 8G | 800% | 1280 | 16 threads, 12 GiB |
| 32 | 14G | 1600% | 2304 | 24 cores, 18 GiB |

The shipped `baremetal-shell.slice` is sized for S = 16 and `MemoryHigh=7G`. For another S, edit the slice (or add `/etc/systemd/system/baremetal-shell.slice.d/size.conf`) **and** `SHELL_MAX_SESSIONS`, and keep the launcher's VM cap at least as large. The broker's own `MemoryMax=512M` and `TasksMax=64` do not depend on S for the byte-relay design (frames are at most 16 KiB; per session buffers are bounded), but watch it on the first load test. If sessions share cores with other work, remember the CPU quota is a ceiling, not a reservation.

## Host egress firewall

`deploy/shell/nftables/host-egress.nft` is a default-drop table: input only SSH from the admin addresses (rate-limited), ICMP basics and established traffic; output only DNS, NTP and an `update_hosts` set (package mirror and Firecracker/GitHub addresses, ideally empty outside upgrade windows); forward dropped. Replace every address marked TODO (the shipped values are documentation addresses) and load it with `nft -f`. Syntax was checked with `nft -c`; behaviour on a real interface was not tested. If the web app ever runs on another machine (see the Unix socket conflict above), the broker-facing transport needs its own rule.

Guests do not depend on this table: by default they have no NIC, and lab guests live in their own network namespace below.

## Networking for lab VMs

Default: **no NIC**. A lesson whose target can live on loopback inside the same guest (a process on 127.0.0.1) uses that and gets nothing else. This is the preferred pattern.

For labs that need a separate target machine:

- One network namespace per session, created by the launcher (nested under the launcher's own empty network namespace, because the unit has `PrivateNetwork=yes`; there is no path to the host's network). The namespace contains `lo`, one tap for the learner VM (`tap-l`, host side 10.77.0.1/30, guest 10.77.0.2) and one for the target VM (`tap-t`, 10.77.1.1/30, guest 10.77.1.2). There is no uplink and no default route.
- Inside the namespace `deploy/shell/nftables/lab-session.nft` applies: IPv6, broadcast, multicast and spoofed sources dropped in prerouting; `input` and `output` default-drop (the guest cannot talk to the namespace's own addresses, which still answer ARP); `forward` default-drop with one accept: learner to target, new TCP connections to the lab's ports, rate-limited, plus established replies; a packet-rate cap per tap; everything else is counted and dropped (other sessions, 169.254.0.0/16 including the metadata address, RFC1918, the internet, DNS, UDP, ICMP). Sysctls inside the namespace: `net.ipv4.ip_forward=1`, `rp_filter=1`, `net.ipv6.conf.all.disable_ipv6=1`.
- The learner's kernel has IPv6, netfilter and packet sockets compiled out (see the kernel section), so a lab that needs raw packets needs a different guest image, not a looser firewall.
- Enabling this needs the launcher drop-in `deploy/shell/systemd/launcher-lab-network.conf.example` (adds `AF_NETLINK`, `CAP_NET_ADMIN`, `/dev/net/tun`, the `net` namespace type). Do not install it on a host that runs no networked lab. The launcher code that creates namespaces and taps does not exist yet (TODO-to-confirm).

**Test the ruleset** (verified 30 October 2026 on the development machine, no root): `deploy/shell/nftables/test-lab-session.sh` runs inside `unshare -Urnm`, builds two namespaces with veth pairs standing in for the taps, loads the ruleset, and checks eleven cases: learner reaches the target on an allowed port (pass); a non-listed port, new target-to-learner connections, the namespace address, 169.254.169.254, 192.168.1.1, 8.8.8.8:53, ICMP to target and gateway, and a spoofed source all fail; the default-drop counter moves. All 11 passed; with the ruleset not loaded, 6 of the checks fail, so the test does discriminate. It validates the ruleset, not the Firecracker tap wiring. Repeat the same list **from inside a real guest** in the hostile suite.

## Guest kernel

`scripts/shell/build-kernel.sh` builds `vmlinux` from the pinned tarball `linux-6.1.191.tar.xz` (sha256 `02f15b77d213e15607923832ef9382e2414ea90df6c007f15a910e139645c35d`, taken from kernel.org's `sha256sums.asc`, whose PGP signature by the kernel.org autosigner key `B8868C80BA62A1FFFAF5FDA9632D3A06589DA6B1` verified on 30 October 2026; the key was fetched over the network and its fingerprint not independently cross-checked). The base is Firecracker v1.17.0's `microvm-kernel-ci-x86_64-6.1.config`, vendored in `deploy/shell/kernel/` and hash-checked. The script then switches off, among others: user namespaces, io_uring, the bpf syscall, kexec, debugfs, userfaultfd, sysrq, binfmt_misc, FUSE, overlayfs, `/dev/mem` and `/dev/port`, MSR/CPUID devices, 32-bit emulation, vsyscall, IPv6, netfilter, packet sockets, SysV IPC, audit; it keeps no modules, vsock, virtio-blk/mmio, ext4, devtmpfs, devpts (via Unix98 PTYs), tmpfs, proc, sysfs, the 8250 console and the loopback/virtio-net stack. After `make olddefconfig` it **asserts the resolved `.config`**, so a dependency that silently re-enables something fails the build. `PERF_EVENTS` cannot be disabled on x86 in 6.1 (selected unconditionally); unprivileged perf stays restricted by the default `kernel.perf_event_paranoid=2`.

Verified: `build-kernel.sh --config-only` downloads the tarball, checks the hash before extracting, resolves the configuration and passes all assertions (about 6 seconds). **Not run:** the actual compile, so nobody has booted this kernel. The reductions are plausible but unproven against `/init`, the agent and the broker checks; boot it on a disposable host and run `scripts/shell/broker-checks.mts` against it before trusting it. Because the build is deterministic only for the same toolchain and work path, the output includes a `MANIFEST` with the compiler version; store its `vmlinux` sha256 in your records and compare on the host after every install.

Firecracker v1.17.0 also ships 6.18 configs. This repository pins 6.1 because the prototype was verified on 6.1; moving to 6.18 is a separate evaluation.

## Logging, monitoring and abuse alerts

Everything goes to journald (stdout as JSON lines). Terminal content is never logged; the broker audit line has session id, account hash, counters and the end reason; the launcher logs `vm_start`, `vm_end` (with reason and lifetime), `refused` and `destroy_failed`. If you ship these logs off-host or keep them beyond the journal rotation, that is stored data: update `content/legal/` and `SITE.legal.lastUpdated` (project rule).

Journal retention: set `SystemMaxUse=` and `MaxRetentionSec=` so a flood of session logs cannot fill the host's disk.

Alert on:

- `destroy_failed`, or `ls /var/lib/baremetal-shell/jail/firecracker | wc -l` greater than the number of live sessions for more than five minutes (an orphan chroot or cgroup). The launcher reaps at start; a recurring orphan is a bug.
- Either unit restarting (`NRestarts` rising, or three starts in ten minutes). A launcher restart ends every session by design.
- `vm_end` reasons in unusual proportion: `cap`, `idle` or `output_cap` rising sharply, or one account hash producing many `start_failed` and `busy` events (probing for the cap).
- Slice pressure: `memory.events` `high`/`max`/`oom_kill` in `/sys/fs/cgroup/baremetal-shell.slice/` nonzero; `pids.events` `max` nonzero; `cpu.stat` `nr_throttled` growing steadily for the whole slice (capacity, not abuse).
- Kernel log: `kvm:`, `BUG:`, `Oops`, `general protection`, `nf_conntrack: table full`, any `Firecracker` seccomp violation (`audit: type=1326` / `SIGSYS`). A seccomp kill of a VMM is a possible escape attempt; preserve the journal and the chroot (disable cleanup) before anything else.
- Firewall: counts of `bm-host-in-drop` and `bm-host-out-drop` log lines. Outbound drops on a host that should initiate nothing are interesting by themselves.
- Disk: free space under `/var/lib/baremetal-shell` and the journal.
- A synthetic probe: every five minutes open a session through the web path and expect `READY` within 5 seconds.

## Patch and upgrade policy

- **Firecracker.** Subscribe to the Firecracker GitHub security advisories and release feed. Security releases: deploy within 48 hours for anything affecting the VMM or jailer, otherwise within the normal monthly window. To upgrade: update `FC_VERSION` and `FC_TARBALL_SHA256` in `install-host.sh` (take the hash from the release's `.sha256.txt` and cross-check the archive's own `SHA256SUMS`), refresh the vendored kernel base config for the new tag, run `install-host.sh` (installs side by side and flips `current`), then restart the launcher, which ends all sessions: drain by stopping new sessions at the web layer first and waiting up to `SHELL_MAX_SECONDS`. Keep the previous version directory for rollback (`ln -sfn`).
- **Guest kernel.** 6.1 stable point releases come out roughly weekly. Rebuild at least monthly and immediately for a CVE in code that is compiled in (check against the resolved `config.used`, not the stock distribution list). Update `KERNEL_VERSION` and `KERNEL_SHA256` together after verifying `sha256sums.asc`, build on the build machine, run the hostile suite, install `vmlinux`, restart the launcher. Check kernel.org's release page for the 6.1 end-of-life date (listed as December 2027 at the time of writing; verify) and plan the move before then.
- **Host kernel, microcode, systemd, nftables.** Apply security updates at least weekly; KVM and CPU side-channel fixes need a reboot, so schedule a drain window rather than deferring. Re-run `install-host.sh --dry-run` after a major upgrade to see if a check regressed (cgroup layout, systemd version, SMT).
- **Rootfs.** Rebuilt from host binaries by `build-rootfs.sh`; rebuild when bash or coreutils get security fixes. It contains no secrets and nothing persistent.
- Always run `scripts/shell/hostile/` against a staging host with the new component before production.

## Backup

Nothing to back up. The host holds no learner data and no secrets: the rootfs and kernel are rebuilt from the repository, the broker has no database, session files are destroyed with the VM. Keep, off-host, only: the repository, the kernel `MANIFEST`/`SHA256SUMS`, any `/etc/baremetal-shell/*.env` overrides, and your host firewall addresses. Rebuilding the host from `install-host.sh` is the recovery procedure.

## Pre-launch checklist

Run on the real host (or an identical staging host) after the install order above. Hostile tests live in `scripts/shell/hostile/` (being written by another agent; this list references that directory and does not duplicate it).

1. `systemd-analyze verify` and `systemd-analyze security baremetal-shell-launcher.service baremetal-shell-broker.service` show no unknown directives; the launcher's exposure is the documented level (offline score 2.4 at writing) and the broker's is lower.
2. `stat -c '%U %G %a' /dev/kvm` prints `root root 600`. The broker's user cannot open it (`sudo -u baremetal-shell-broker test -r /dev/kvm` fails).
3. Both services start; `systemctl show -p User,CapabilityBoundingSet,NoNewPrivileges baremetal-shell-broker` shows an empty capability set; the launcher socket is `0660 root:baremetal-shell-launch`, the broker socket `0660 baremetal-shell-broker:baremetal-shell-client`, and the web user (and nobody else) can connect to the latter.
4. The cgroup checks from "First-start checks" pass (VM cgroups under the slice, `memory.max` 8G).
5. `scripts/shell/broker-checks.mts` passes against the production kernel and rootfs.
6. All of `scripts/shell/hostile/` passes: fork bomb, memory bomb, disk fill, busy loop (session ends, host unaffected, slice not throttled beyond its ceiling); guest has no route to the host, other sessions, link-local, RFC1918, the internet or DNS; vsock reaches only the broker; the guest cannot open `/dev/kvm`, cannot see host processes, files or environment; broker fuzzing (oversized and malformed frames, slow readers, hung agents); teardown (kill the web app, the broker, the launcher and a VM at random: no orphan VMs, taps, namespaces, chroots or cgroups afterwards; reaper on start works).
7. `nft list ruleset` on the host matches `host-egress.nft`; `deploy/shell/nftables/test-lab-session.sh` passes; the same cases pass from inside a real guest.
8. The host has no swap, KSM is off, SMT policy is applied and recorded, and `mitigations=off` is not on the kernel command line.
9. Sizing: run `S` simultaneous sessions with the busy-loop workload and confirm the slice stays under its limits and the host stays responsive.
10. A learner account cannot open a second session; the (S+1)-th concurrent session is refused cleanly.
11. The launcher cap (`maxVms`) is at least `S`, and the web endpoint, quotas and `content/legal/` changes are in place (separate work).
12. Alerts above are wired and each was triggered once on purpose (kill a VM's cgroup, fill the disk quota, stop the broker).
13. Write down what was **not** tested and who accepted it: a hypervisor- or CPU-level escape is out of scope for tests and remains the accepted residual risk, mitigated by the empty host and the patch policy.

## Verified and unverified (30 October 2026)

Verified here, without root and without booting a VM:

- The three unit files parse under `systemd-analyze verify` (systemd 262) with no warnings; `systemd-analyze security --offline`: launcher 2.4 (OK), broker 0.4 (SAFE).
- `udevadm verify` accepts the udev rule; `systemd-tmpfiles --dry-run` parses the tmpfiles entry (group and user lookups fail only because the accounts do not exist on the development machine).
- `install-host.sh --dry-run` runs as a normal user and prints every action; non-dry-run refuses to run unprivileged; `sh -n` is clean (shellcheck is not installed here).
- The Firecracker v1.17.0 tarball hash (`06094a11...e558`) was downloaded and matched the published `.sha256.txt`; the archive's internal `SHA256SUMS` match; both binaries are static-pie with no interpreter.
- The kernel tarball hash comes from the PGP-signed `sha256sums.asc`; the configuration step passes its assertions.
- `lab-session.nft` and `host-egress.nft` pass `nft -c`; the lab ruleset passed its behavioural test in user and network namespaces.

Not verified: any real VM start under the units (capabilities, seccomp list, MDWE, device cgroup, delegated cgroup layout); the kernel compile and boot; `install-host.sh` actual (root) execution; the launcher entry point and its environment names; vsock socket handover between launcher and broker; lab tap creation; host firewall behaviour on real interfaces; behaviour under hostile workloads.
