#!/bin/bash
# READ-ONLY host-side inspection of every running jailed microVM: uid/gid, chroot, seccomp, capabilities, pid
# namespace, cgroup limits, /dev/kvm permissions. Changes nothing. Run as root WHILE a session is open
# (the driver runs it for you; by hand: start a session, then run this in another terminal).
# Each check prints PASS/FAIL like the other suite scripts and the exit status is non-zero on any FAIL.
# Expected values come from src/lib/shell/vm.ts defaults; override with the variables below if you changed them.
set -u
[ "$(id -u)" = 0 ] || { echo "run as root (reads /proc/<pid>/root, ns and limits)" >&2; exit 2; }
FIRECRACKER=${FIRECRACKER:-$(command -v firecracker || echo firecracker)}
NAME=$(basename "$FIRECRACKER")
CGROOT=${SHELL_CGROUP_ROOT:-/sys/fs/cgroup}
CHROOT_BASE=${SHELL_CHROOT_BASE:-/srv/baremetal-jailer}
UID_EXPECT=${SHELL_VM_UID:-300001}
CPU_MAX_EXPECT=${BM_CPU_MAX:-50000 100000}
MEM_MAX_EXPECT=${BM_MEM_MAX:-419430400}       # 400M
PIDS_MAX_EXPECT=${BM_PIDS_MAX:-64}
fails=0
pass() { echo "PASS  $1"; }
fail() { echo "FAIL  $1  $2"; fails=$((fails + 1)); }
check() { if [ "$2" = ok ]; then pass "$1"; else fail "$1" "$3"; fi; }
field() { awk -v k="$2:" '$1 == k { $1 = ""; sub(/^ /, ""); print; exit }' "$1"; }   # field <status-file> <Key>

# /dev/kvm on the host: must not be world accessible (the dev machine has 0666).
if [ -e /dev/kvm ]; then
  mode=$(stat -c %a /dev/kvm); own=$(stat -c %U:%G /dev/kvm)
  [ $((8#$mode & 8#007)) -eq 0 ] && check "host /dev/kvm not world-accessible ($mode $own)" ok || check "host /dev/kvm not world-accessible" bad "mode $mode owner $own: any local user can create VMs; restrict to the jailer group/uid"
else
  fail "host /dev/kvm exists" "missing"
fi

dirs=$(ls -d "$CGROOT/$NAME"/bm-* 2>/dev/null)
[ -n "$dirs" ] && pass "found running VM cgroups" || { fail "found running VM cgroups" "none under $CGROOT/$NAME: open a session first"; exit 1; }

for cg in $dirs; do
  id=$(basename "$cg"); echo "--- $id"
  pids=$(cat "$cg/cgroup.procs")
  [ -n "$pids" ] && pass "$id: cgroup has processes ($(echo $pids | wc -w))" || fail "$id: cgroup has processes" "empty"
  check "$id: cpu.max = $CPU_MAX_EXPECT" "$([ "$(cat "$cg/cpu.max")" = "$CPU_MAX_EXPECT" ] && echo ok)" "got '$(cat "$cg/cpu.max")'"
  check "$id: memory.max = $MEM_MAX_EXPECT" "$([ "$(cat "$cg/memory.max")" = "$MEM_MAX_EXPECT" ] && echo ok)" "got '$(cat "$cg/memory.max")'"
  check "$id: pids.max = $PIDS_MAX_EXPECT" "$([ "$(cat "$cg/pids.max")" = "$PIDS_MAX_EXPECT" ] && echo ok)" "got '$(cat "$cg/pids.max")'"
  check "$id: memory.swap.max = 0 (no swap escape of the memory limit)" "$([ "$(cat "$cg/memory.swap.max" 2>/dev/null || echo 0)" = 0 ] && echo ok)" "got '$(cat "$cg/memory.swap.max" 2>/dev/null)' (INFO if the host has no swap)"
  for pid in $pids; do
    st=/proc/$pid/status; [ -r "$st" ] || continue
    comm=$(cat /proc/$pid/comm); echo "  pid $pid ($comm)"
    p="$id/$pid"
    uids=$(field $st Uid); gids=$(field $st Gid)
    check "$p: real/effective/saved/fs uid all $UID_EXPECT" "$([ "$uids" = "$UID_EXPECT $UID_EXPECT $UID_EXPECT $UID_EXPECT" ] && echo ok)" "Uid: $uids"
    check "$p: gids all $UID_EXPECT" "$([ "$gids" = "$UID_EXPECT $UID_EXPECT $UID_EXPECT $UID_EXPECT" ] && echo ok)" "Gid: $gids"
    check "$p: no supplementary groups" "$([ -z "$(field $st Groups | tr -d ' ')" ] && echo ok)" "Groups: $(field $st Groups)"
    for c in CapInh CapPrm CapEff CapBnd CapAmb; do
      v=$(field $st $c); check "$p: $c empty" "$([ "$v" = 0000000000000000 ] && echo ok)" "$c=$v (a capability is left on the VMM)"
    done
    check "$p: NoNewPrivs = 1" "$([ "$(field $st NoNewPrivs)" = 1 ] && echo ok)" "NoNewPrivs=$(field $st NoNewPrivs)"
    check "$p: Seccomp = 2 (filter mode)" "$([ "$(field $st Seccomp)" = 2 ] && echo ok)" "Seccomp=$(field $st Seccomp) (0=none, 1=strict, 2=filter). The jailer/Firecracker filter is missing"
    nf=$(field $st Seccomp_filters); [ -n "$nf" ] && check "$p: at least one seccomp filter attached ($nf)" "$([ "$nf" -ge 1 ] && echo ok)" "Seccomp_filters=$nf"
    root=$(readlink /proc/$pid/root)
    if [ "$root" = / ]; then fail "$p: process is chrooted" "root is / (not chrooted)"
    else case "$root" in */"$id"/root) pass "$p: chroot is .../$id/root";; *) fail "$p: chroot path" "root=$root";; esac; fi
    cwd=$(readlink /proc/$pid/cwd); check "$p: cwd inside the chroot ($cwd)" "$([ "$cwd" = / ] && echo ok)" "cwd=$cwd"
    check "$p: pid namespace differs from host init" "$([ "$(readlink /proc/$pid/ns/pid)" != "$(readlink /proc/1/ns/pid)" ] && echo ok)" "same pid ns as PID 1 ($(readlink /proc/$pid/ns/pid)): jailer --new-pid-ns not in effect"
    nspid=$(field $st NSpid); [ "$(echo $nspid | wc -w)" -ge 2 ] && pass "$p: NSpid shows nested pid namespace ($nspid)" || fail "$p: NSpid nested" "NSpid=$nspid"
    check "$p: mount namespace differs from host init" "$([ "$(readlink /proc/$pid/ns/mnt)" != "$(readlink /proc/1/ns/mnt)" ] && echo ok)" "shares the host mount ns"
    if [ "$(readlink /proc/$pid/ns/net)" = "$(readlink /proc/1/ns/net)" ]; then
      echo "INFO  $p: shares the host network namespace (expected only while VMs have no NIC; with the lab network the VMM must be in its own netns)"
    else pass "$p: own network namespace"; fi
    echo "INFO  $p: user ns: $(readlink /proc/$pid/ns/user) (host: $(readlink /proc/1/ns/user))"
    echo "INFO  $p: exe: $(readlink /proc/$pid/exe)  threads: $(field $st Threads)  state: $(field $st State)"
    echo "INFO  $p: rlimits: $(awk '/Max (open files|processes|file size|address space)/' /proc/$pid/limits | tr -s ' ' | tr '\n' ';')"
    # Open files: only the kvm/vm/vcpu anon inodes, the vsock unix socket, the config/pipes, the chroot's files. A host path outside the chroot is a leak.
    outside=""; for fd in /proc/$pid/fd/*; do t=$(readlink "$fd" 2>/dev/null) || continue; case "$t" in /*) case "$t" in "$CHROOT_BASE"/*|/dev/kvm|/dev/null|/dev/urandom|/dev/zero|/vmlinux|/rootfs.ext4|/vm.json|/v.sock*) ;; *) outside="$outside $t";; esac;; esac; done
    [ -z "$outside" ] && pass "$p: no open files outside the chroot" || fail "$p: open files outside the chroot" "$outside"
    echo "INFO  $p: fd types: $(for fd in /proc/$pid/fd/*; do readlink "$fd" 2>/dev/null; done | sed -E 's/[0-9]+/N/g' | sort | uniq -c | tr -s ' ' | tr '\n' ';')"
    mounts=$(wc -l < /proc/$pid/mountinfo); echo "INFO  $p: mountinfo lines: $mounts (review: only the chroot + /dev nodes expected)"
  done
  # Chroot contents: owned by the jailer uid, no host binaries besides the VMM, /dev/kvm node permissions.
  root=$CHROOT_BASE/$NAME/$id/root
  if [ -d "$root" ]; then
    echo "INFO  $id: chroot listing: $(ls -A "$root" | tr '\n' ' ')"
    if [ -e "$root/dev/kvm" ]; then
      m=$(stat -c %a "$root/dev/kvm"); o=$(stat -c %u:%g "$root/dev/kvm")
      check "$id: chroot /dev/kvm not world-accessible ($m $o)" "$([ $((8#$m & 8#007)) -eq 0 ] && echo ok)" "mode $m"
    fi
    [ "$(stat -c %u "$root")" = "$UID_EXPECT" ] && pass "$id: chroot root owned by $UID_EXPECT" || fail "$id: chroot root owner" "$(stat -c %u "$root")"
    [ "$(stat -c %a "$CHROOT_BASE")" ] && echo "INFO  chroot base mode: $(stat -c '%a %U:%G' "$CHROOT_BASE") (expected 700 root:root or 755 root:root; never writable by the VM uid)"
    wr=$(find "$CHROOT_BASE" -maxdepth 2 -perm /o+w ! -type l 2>/dev/null | head -5); [ -z "$wr" ] && pass "$id: nothing world-writable under the chroot base (depth 2)" || fail "$id: world-writable paths" "$wr"
    # Hard links to a shared image would let the guest-side VMM modify it; the broker copies per session.
    rf=$root/rootfs.ext4; [ -e "$rf" ] && { l=$(stat -c %h "$rf"); [ "$l" = 1 ] && pass "$id: rootfs is a private copy (link count 1)" || fail "$id: rootfs link count" "$l: shared with another path"; }
  else
    fail "$id: chroot exists" "$root missing"
  fi
done
echo; [ "$fails" -eq 0 ] && echo "all jailer inspection checks passed" || echo "$fails jailer inspection check(s) FAILED"
exit $((fails > 0))
