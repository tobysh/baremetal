# Jailer and VMM containment checklist (manual, read-only)

Documented inspection steps, not exploit code. Start ONE session (for example `SHELL_MODE=jailed` broker plus a client, or `driver.mts --only jailer-inspect` and read the log), then run these as root on the host. `inspect-jailer.sh` automates sections 1 to 6 and exits non-zero on a deviation; do the rest by hand. Nothing here changes any state.

Setup used below (adjust to your paths):

```sh
NAME=$(basename "$FIRECRACKER")                  # the jailer names chroot and cgroup after the exec file
CG=/sys/fs/cgroup/$NAME ; BASE=${SHELL_CHROOT_BASE:-/srv/baremetal-jailer}
ID=$(ls $CG | grep '^bm-' | head -1)             # one live VM
PID=$(head -1 $CG/$ID/cgroup.procs)              # the Firecracker VMM (host pid; there is a new pid ns)
```

| # | Check | Command | Expected |
|---|---|---|---|
| 1 | Unprivileged uid and gid, no groups | `grep -E '^(Uid\|Gid\|Groups):' /proc/$PID/status` | `Uid` and `Gid` rows are four copies of `SHELL_VM_UID` (default 300001); `Groups:` empty. Not 0. |
| 2 | No capabilities | `grep -E '^Cap(Inh\|Prm\|Eff\|Bnd\|Amb):' /proc/$PID/status` | All five `0000000000000000`. |
| 3 | no_new_privs and seccomp on the VMM | `grep -E '^(NoNewPrivs\|Seccomp\|Seccomp_filters):' /proc/$PID/status` | `NoNewPrivs: 1`, `Seccomp: 2`, `Seccomp_filters: 1` or more. `Seccomp: 0` means the filter is not loaded (jailer `--seccomp-level`, or Firecracker started with `--no-seccomp`): FAIL. |
| 4 | Chrooted | `readlink /proc/$PID/root; readlink /proc/$PID/cwd` | `.../$NAME/$ID/root` and `/`. `/` for root means no chroot. |
| 5 | New pid namespace | `readlink /proc/$PID/ns/pid /proc/1/ns/pid; grep NSpid /proc/$PID/status` | Different inodes; `NSpid:` has two numbers (host pid, then 1). |
| 6 | Mount namespace separate | `readlink /proc/$PID/ns/mnt /proc/1/ns/mnt; cat /proc/$PID/mountinfo` | Different inodes; few lines, only the chroot. |
| 7 | cgroup limits | `cd $CG/$ID; cat cpu.max memory.max memory.swap.max pids.max` | `50000 100000`, `419430400` (400M), `0` (or swap absent), `64`. `memory.events` shows `oom_kill` after the memory test. |
| 8 | Whole VM is in the cgroup | `cat $CG/$ID/cgroup.procs; cat /proc/$PID/cgroup` | Only Firecracker (and its threads) listed; the cgroup path ends in `$NAME/$ID`. |
| 9 | `/dev/kvm` host permissions | `stat -c '%a %U:%G' /dev/kvm; stat -c '%a %u:%g' $BASE/$NAME/$ID/root/dev/kvm` | Not world-accessible (no `o+rw`; dev machines often have 0666, which is a FAIL on the real host). The chroot node is created by the jailer for the VM uid only. |
| 10 | Open files | `ls -l /proc/$PID/fd` | `anon_inode:kvm-*`, a pipe or two, the unix socket `v.sock`, files inside the chroot. No fd to a host path outside the chroot, no listening TCP socket. |
| 11 | Network namespace | `readlink /proc/$PID/ns/net /proc/1/ns/net; ls /sys/class/net; ip netns list` | Without the lab network: same netns as the host, and no tap was created. With it: the VMM is in its own netns with exactly one tap. After teardown, no new link or netns remains. |
| 12 | Chroot contents and ownership | `ls -la $BASE/$NAME/$ID/root; stat -c '%h' $BASE/$NAME/$ID/root/rootfs.ext4` | Owned by the VM uid; `vmlinux`, `rootfs.ext4`, `vm.json`, `dev/`, `v.sock` only; rootfs link count 1 (a private copy, so a hostile VMM cannot modify the shared image). Nothing under `$BASE` writable by others: `find $BASE -maxdepth 2 -perm /o+w`. |
| 13 | VMM resource limits | `cat /proc/$PID/limits` | Review `Max open files`, `Max processes`, `Max file size`: the jailer's `--resource-limit` defaults leave these generous; pids are bounded by the cgroup. Record the values. |
| 14 | Host sees no guest processes | `ps -eo pid,user,comm \| wc -l` before/after a guest fork bomb | Growth is only VMM threads (the `pids.current` of the cgroup), never one host process per guest process. |
| 15 | Broker service unit (when it exists) | `systemctl show shell-broker -p User,NoNewPrivileges,ProtectSystem,CapabilityBoundingSet` | Deployment item: today the broker runs as root to start the jailer (see "Privilege separation" in the design doc). Record that fact; it must be split before launch. |
| 16 | Kernel and mitigations | `uname -r; cat /sys/devices/system/cpu/smt/control /sys/devices/system/cpu/vulnerabilities/*` | Patched host kernel; SMT policy decided and recorded (the VM config sets `smt: false` for the guest only). |
| 17 | Guest kernel config | in the guest: `ls /sys/module \| head`, `cat /proc/sys/user/max_user_namespaces` | No loadable modules; record whether unprivileged user namespaces are enabled (the `probe` payload reports it). |

A kernel or KVM/Firecracker vulnerability is out of scope here: this checklist verifies that the configured layers are present, not that they are unbreakable.
