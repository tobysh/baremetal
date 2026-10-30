# Hostile-workload verification for the microVM shell

Status: WRITTEN, NOT YET RUN (30 October 2026). This is item 5 of "Not done" in `docs/interactive-shell-isolation.md` (Verification, 1 to 5) and the evidence needed before the shell launches. Nothing in this directory has been executed or shipped on a machine with data on it.

The suite does resource abuse and permission or containment probes only. It contains no kernel, KVM or Firecracker exploit code; an escape at that level is the residual risk the design accepts.

## DANGER

It runs fork bombs, memory bombs, tmpfs fills, CPU spins and output floods inside real microVMs, plus a broker that starts jailed VMs as root. If a limit is wrong the host can hang, swap or OOM. Run it ONLY on a disposable VM with no data, no secrets, no other services, and nothing you want back. `run.sh` refuses to start unless `BM_DISPOSABLE_HOST=yes-this-machine-is-disposable` is set and the user is root, prints a banner, asks you to type the hostname when interactive, and enforces a global timeout (`BM_GLOBAL_TIMEOUT_S`, default 3600 s) with a sweep of leftover VMs afterwards.

## Files

| File | Purpose |
|---|---|
| `run.sh` | Guarded entry point: guards, banner, outer timeout, log and JSON in `BM_RESULTS_DIR`, final sweep. |
| `driver.mts` | The TS driver (tsx). Opens sessions through the broker, feeds payloads, asserts host-side outcomes. |
| `payloads/*.sh` | Guest-side scripts (bash plus the coreutils subset of `build-rootfs.sh`; no compiler). `_lib.sh` is prepended to each. |
| `net/egress.sh` | Guest-side egress tests. Requires the lab-network setup. |
| `probe/bm-probe.c`, `probe/add-probe.sh` | Optional static helper for syscall-level probes (unshare, chroot, setns, ptrace, mknod, vsock-to-host). Built on the host and injected into a COPY of the rootfs. |
| `inspect-jailer.sh` | Read-only host inspection of live jailed VMs (uid, chroot, seccomp, caps, namespaces, cgroup, `/dev/kvm`). |
| `ESCAPE-CHECKLIST.md` | The documented manual jailer checks with expected values. |

## Provision the disposable VM

1. A fresh VM or bare-metal box with KVM exposed (`ls -l /dev/kvm`), cgroup v2 with the `cpu memory pids` controllers (`cat /sys/fs/cgroup/cgroup.controllers`), root access, at least 4 GiB RAM and 2 vCPUs (the memory bound is checked against `MemAvailable`; a tiny host makes it meaningless). Nested virtualisation works but is slower and weaker; note it in the results.
2. Take a snapshot or be ready to destroy it afterwards. Nothing from the dev machine's secrets, `.env.local` or MongoDB goes here.
3. Install Node (the version the repo uses), the repo checkout and `npm ci` (for tsx), `gcc`, `e2fsprogs` (for `mkfs.ext4` and `debugfs`), and `util-linux` (`setpriv`).
4. Use the official static Firecracker and jailer release binaries (the distro build is dynamically linked and cannot exec in the jailer chroot; `run.sh` checks). Pin and checksum them.
5. Build the guest assets the way production would: your own pinned guest kernel (`vmlinux`) and the rootfs from `scripts/shell/build-rootfs.sh`. For the syscall probe, make a probe copy: `scripts/shell/hostile/probe/add-probe.sh rootfs.ext4 rootfs-probe.ext4`. Do not use the probe image anywhere else.
6. `/dev/kvm` must be restricted for a real result: `chgrp`/`chmod 660` it, as in the design doc. The suite FAILs a world-accessible `/dev/kvm` (it is 0666 on the dev machine).
7. `mkdir -p /srv/baremetal-jailer` (mode 755, root). The VM uid is 300001 unless `SHELL_VM_UID` is set.
8. Run `broker-checks.mts` once first as a smoke test (`SHELL_KERNEL=... SHELL_ROOTFS=... FIRECRACKER=... npx tsx scripts/shell/broker-checks.mts`). It also boots the unjailed driver; and first-time-run the jailed driver with `scripts/shell/launch-jailed.sh`, which `startJailedVm` mirrors but which has not run before this suite.

## Run

```sh
export BM_DISPOSABLE_HOST=yes-this-machine-is-disposable
export FIRECRACKER=/opt/fc/firecracker JAILER=/opt/fc/jailer
export SHELL_KERNEL=/opt/fc/vmlinux SHELL_ROOTFS=/opt/fc/rootfs-probe.ext4
scripts/shell/hostile/run.sh --list                  # harmless: the plan
scripts/shell/hostile/run.sh --dry-run               # harmless: also lints the payloads (ASCII, line length, size)
scripts/shell/hostile/run.sh --strict                # everything; a SKIP counts as a failure
scripts/shell/hostile/run.sh --only forkbomb,memhog  # a subset
scripts/shell/hostile/run.sh --skip net-egress
```

Expect about 20 to 30 minutes (each destructive payload waits up to a minute for the broker's absolute cap). Per-test limits are set by the driver: destructive payloads run with a 60 s absolute limit and a 120 s idle limit.

Options and environment: `--socket PATH` runs the payload tests against an already-running broker service instead of the in-process broker (then per-test limits cannot be overridden, timing assertions are relaxed and in-process RSS/fd checks are skipped). Bounds can be overridden for re-judging only: `BM_MAX_HOST_MEM_DROP_MIB` (600 per VM), `BM_MAX_HOST_PROC_GROWTH` (100 per VM), `BM_MAX_BROKER_RSS_GROWTH_MIB` (160), `BM_CPU_SLACK` (1.35), `BM_END_GRACE_S` (20), `BM_TEARDOWN_S` (15), `BM_BOOT_S` (30). Expected cgroup values: `BM_CPU_MAX` ("50000 100000"), `BM_MEM_MAX` (419430400), `BM_PIDS_MAX` (64). Paths: `SHELL_CHROOT_BASE`, `SHELL_CGROUP_ROOT`, `SHELL_VM_UID`.

## Network egress (needs the lab-network setup)

The broker's `startJailedVm` starts VMs with `"network-interfaces": []`, so the egress test cannot be exercised end to end through it. Once the lab-network work gives sessions a NIC on a per-session tap in its own netns with default-drop nftables, run the service with that configuration and:

```sh
BM_LAB_NETWORK=yes BM_HOST_ADDRS="10.200.0.1 192.0.2.10" BM_OTHER_ADDRS="10.200.1.2" \
  BM_TAP=<tap name, optional> BM_STRICT=1 scripts/shell/hostile/run.sh --only net-egress --socket /run/baremetal-shell/broker.sock
```

Without `BM_LAB_NETWORK=yes` the test is reported as SKIP ("requires the lab-network setup"). With it, a guest without a NIC is a FAIL. The guest probes (TCP via `/dev/tcp`, UDP via `/dev/udp`): every host address on common ports plus the canary port, the gateway, other sessions' addresses, link-local and metadata (169.254.169.254, 169.254.170.2, fd00:ec2::254, fe80::1), RFC1918 and CGNAT, the internet (1.1.1.1, 8.8.8.8, 9.9.9.9, an IPv6 address, DNS and NTP over UDP), and broadcast, multicast and DHCP sends. Expected result: all blocked. Evidence on the host side is stronger than the guest's word: the driver runs TCP and UDP canary listeners on `0.0.0.0:BM_CANARY_PORT` and any hit fails. `BM_STRICT=1` also fails a fast refusal (RST/ICMP) toward host classes, because only a silent drop proves the packet never reached a host stack. What bash cannot do (raw ARP, a real DHCP DISCOVER from port 68) must be checked by capture while the test runs: `tcpdump -ni <tap>` (expect only ARP for the gateway at most, nothing from the guest to other destinations) and `nft list ruleset` drop counters rising, plus `ip -s link show <tap>`.

## What each test does

Guest-side PASS/FAIL lines (`BM_CHECK`) come from the guest and are advisory (a hostile guest can print anything); the host-side assertions are the verdicts.

| Test | Hostile input | Host-side assertions |
|---|---|---|
| `forkbomb` | `bomb(){ bomb\|bomb& }` as the learner; also tries to raise its own RLIMIT_NPROC | Ended by the broker or VM within the absolute limit plus grace; cgroup `pids.current` <= `pids.max`; host process count growth bounded; memory and CPU bounds; no leftovers; broker admits a fresh session. |
| `memhog` | three processes doubling a string until the guest OOMs | cgroup memory peak <= `memory.max` (400 MiB); host `MemAvailable` drop bounded; OOM kills recorded; teardown and health. |
| `tmpfill` | fill `/tmp` and `/home/learner` (32 MiB tmpfs each), 150k inodes, write to the read-only root | Guest sizes bounded; host memory bound holds (tmpfs inodes are not counted by `size=`); root is read-only; clean teardown. |
| `cpuspin` | four busy loops, 20 s | VM CPU over the window <= quota x 1.35 (cpu.max 50000/100000 = 0.5 core), `nr_throttled` > 0. The broker does not currently kill a CPU-bound session (the design wants it to); the result is "contained", not "ended". |
| `outflood` | unbounded output | Cut at the output cap with `output_limit`, client bytes <= cap; slow-reader variant (client stops reading): backpressure keeps broker RSS and fds bounded and the absolute limit still ends the session. |
| `ptyabuse` | 60 KB and 180 KB lines, 300 KB random bytes, NULs, clear/reset, OSC 0/52 (clipboard write and read), device and cursor queries, alt screen and mouse modes, huge cursor moves and resize requests, bad UTF-8, bidi overrides, C1 controls, tty ioctls | Session survives, frames stay within bounds, shell responsive afterwards. |
| `input-abuse` | 60 KB line without newline, 32 KB binary garbage in, 240 KB input burst, 3000 resize frames, eight kinds of client protocol violation | Responsive after the long line, garbage and resize storm; `input_rate` ends the burst; every violation ends the session; no leftovers. |
| `devices` | open `/dev/kvm`, `mem`, `kmem`, `port`, `kmsg`, `vda`, `vhost-*`, `/proc/kcore`, `/proc/kmsg`, `/proc/sysrq-trigger` (open only, nothing written) | Every open refused or absent; no dangerous node writable. |
| `mounts-ns` | `mount` tmpfs, bind, remount rw, proc, sysfs, move; `umount`; presence of chroot, unshare, nsenter, su, sudo, compilers, interpreters, nc, curl, setuid files; `uid_map` write | All refused; tools and setuid files absent; root mounted ro. |
| `procsys` | write-open every file under `/proc/sys`, `/sys/kernel`, `/sys/class`; read `/proc/cpuinfo`, cmdline, mounts, environ | Nothing writable; no host paths or secret-looking strings visible; cpu model and hypervisor details recorded as INFO. |
| `ptrace` | open `/proc/1/mem`, `environ`, `syscall`, `stack`, `root`, `cwd`, `fd`, `ns/*` of PID 1 and the root agent | All refused (no `ptrace(2)` binary exists; the real `PTRACE_ATTACH` is in the probe). |
| `privesc` | ids, groups, capability sets, `NoNewPrivs`, `setpriv --reuid=0` and capability grabs, a setuid copy of `id`, `/etc/shadow`, `/root` | Learner stays uid 1000 with empty caps and no_new_privs; everything refused. |
| `signal-init` | `kill -s` with ten signals at PID 1 and the agent | All refused; both still alive. |
| `probe` | static helper: setuid(0), unshare, chroot, pivot_root, mount, setns on PID 1's mnt ns, `PTRACE_ATTACH`/`SEIZE` on PID 1, mknod of /dev/mem and a block device, reboot, kexec/module loading, iopl, clock/hostname, raising rlimits, raw and packet sockets, open `/dev/kvm`/`mem`/`port`, vsock connect to the HOST (CID 2) on 13 ports, user namespace creation (INFO) | Everything refused; vsock to the host reaches nothing. SKIP without the probe rootfs. |
| `rapid-reconnect` | 30 open-then-vanish clients, 100 same-account reconnects, 60 silent sockets, 5 simultaneous accounts against a cap of 3 | Never more than the cap of VMs alive at once, `busy`/`account_busy` answers, silent sockets dropped by the open timeout, fds back to baseline, no leftovers. |
| `jailer-inspect` | (live session) runs `inspect-jailer.sh` | See `ESCAPE-CHECKLIST.md` sections 1 to 12; exit 0. |
| `broker-crash-reap` | SIGKILL the real service with a live VM, restart (reaper), SIGTERM with a live VM | Orphans recorded; restart reaps them; SIGTERM leaves nothing; new sessions work. |
| `net-egress` | see above | Canary silent, all guest probes blocked. |

After EVERY test the driver also checks, against a baseline taken before it: no leftover `firecracker`/`jailer` processes, chroot directories, VM cgroups, network links (taps), named netns and namespaces still referenced by any process, or mounts under the chroot base; `/dev/kvm` mode unchanged; broker capacity released within `BM_TEARDOWN_S`; and a fresh session starts and runs a command. A final pass repeats the leftover check for the whole suite.

## Reading results

Lines are `PASS|FAIL|INFO|SKIP  <test>: <check>  -- <detail>`. Only FAIL fails the run (exit 1); with `--strict` SKIP does too. INFO lines are measurements to copy into the record (peak memory, end reasons, kernel facts, `userns` enabled or not). The full JSON is in `$BM_RESULTS_DIR/run-<stamp>.json` and the log next to it (default `/var/tmp/bm-hostile`). `run.sh` exits non-zero if the run failed or VM cgroups remain after its sweep. If the host wedges, the global timeout is meant to fire first; destroy the VM, do not repair it.

Triage order for a FAIL: a host-bound failure (memory, pids, CPU, leftovers, session not ended) is a launch blocker; a guest-side FAIL means the image or agent allows something it should not; a `probe`/`procsys` INFO about user namespaces or CPU flags is a decision, not a failure.

## Pass criteria

Launch is not justified until a `--strict` run on the real host class shows, with the production kernel, rootfs and binaries:

1. Zero FAIL and zero SKIP (so the probe image and the lab-network tests ran).
2. Every destructive payload (`forkbomb`, `memhog`, `cpuspin`, `outflood`, slow reader) was contained inside the stated bounds: cgroup memory <= 400 MiB, `pids.current` <= 64, VM CPU <= 0.5 core x 1.35, host `MemAvailable` drop <= 600 MiB per VM, host process growth <= 100 per VM, broker RSS growth <= 160 MiB, and the session ended by the cap or the VM, within the absolute limit plus 20 s.
3. No leftover process, chroot, cgroup, tap, namespace or mount after any test and after the final pass, and the broker admitted a fresh session after each test.
4. `jailer-inspect` and the manual checklist match the expected values, including a non-world-accessible `/dev/kvm`.
5. Network egress: every probe blocked, canary silent, host capture shows nothing forwarded.
6. Known gaps (below) are either fixed or accepted in writing in `docs/security.md`.

Known gaps this suite exposes rather than fixes: the broker does not end a session for sustained 100% CPU (design wants it), `startJailedVm` has no NIC support, the broker runs as root (privilege separation), and per-job guest pid cgroups do not exist (the guest relies on RLIMIT_NPROC 256 from the agent).

## Results record (paste into `docs/security.md`)

```md
### Shell hostile-workload verification (YYYY-MM-DD)

- Host: <provider/type>, kernel <uname -r>, <vCPUs>/<RAM>, KVM <bare metal | nested>, cgroup v2, SMT <on/off>, mitigations <state>
- Firecracker <version> / jailer <version> (static, sha256 <...>), guest kernel <version/hash>, rootfs <hash>, probe image <yes/no>
- Repo commit <sha>, broker limits <maxSessions/idle/absolute/output cap>, cgroup <cpu.max, memory.max, pids.max>
- Command: `run.sh --strict`  Result: <N PASS / N FAIL / N SKIP>  Log: <path or attached>

| Area | Result | Measured | Bound |
|---|---|---|---|
| Fork bomb | pass/fail | guest procs <n>, cgroup pids peak <n>, ended by <reason> in <s> | pids <= 64, ended <= 80 s |
| Memory bomb | | cgroup peak <MiB>, host MemAvailable drop <MiB>, oom_kill <n> | <= 400 MiB / 600 MiB |
| tmpfs / inode fill | | /tmp <MiB>, home <MiB>, host memory drop <MiB> | <= 40 MiB each |
| CPU spin | | <cores> over <s>, nr_throttled <n> | <= 0.68 core |
| Output flood / slow reader | | cut at <bytes>, broker RSS growth <MiB> | cap, <= 160 MiB |
| PTY abuse / input abuse | | | session survives |
| Device / mount / ptrace / privesc / signal / procsys | | <n> checks, <n> INFO | all refused |
| Syscall probe (incl. vsock to host) | | userns <enabled/disabled> | all refused |
| Rapid reconnect | | max concurrent VMs <n>, fd growth <n> | <= cap, <= 12 |
| Jailer inspection | | uid <n>, Seccomp <n> filters <n>, caps 0, /dev/kvm <mode> | per checklist |
| Broker crash / reaper | | orphans after SIGKILL <n>, reaped <n> | 0 left |
| Network egress | pass/fail/not run | canary hits <n> | 0 |
| Leftovers after suite | | processes/chroots/cgroups/taps/netns/mounts | 0 |

Open findings: <list, with severity and ticket>. Accepted residual risk: kernel/KVM/Firecracker escape (out of scope), CPU side channels (host policy).
```
