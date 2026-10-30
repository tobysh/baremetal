# Mounts, namespaces and the userland that would help an attacker. Syscall-level chroot/unshare/setns are in bm-probe.
bm_denied mounts.tmpfs mount -t tmpfs t /mnt
bm_denied mounts.bind mount --bind /tmp /mnt
bm_denied mounts.remount-rw mount -o remount,rw /
bm_denied mounts.remount-rw-ro-bind mount -o remount,bind,rw /home/learner
bm_denied mounts.proc mount -t proc p /mnt
bm_denied mounts.sysfs mount -t sysfs s /mnt
bm_denied mounts.umount-proc umount /proc
bm_denied mounts.umount-tmp umount /tmp
bm_denied mounts.umount-pts umount /dev/pts
bm_denied mounts.move mount --move /tmp /mnt
for c in chroot unshare nsenter su sudo doas pkexec newgrp sg setcap getcap capsh gcc cc c99 tcc clang as ld make python python3 perl ruby node php gdb strace ltrace nc ncat socat curl wget ssh telnet busybox insmod modprobe kexec debugfs mknod chmod chown; do
  if bm_has_cmd "$c"; then bm_fail "mounts.no-tool:$c" "present at $(type -P "$c")"; else bm_pass "mounts.no-tool:$c" "absent"; fi
done
# Setuid / setgid / file-capability candidates anywhere on the image.
sug=""; for f in /bin/* /lib/* /lib64/* /init /agent /etc/*; do [ -f "$f" ] && { [ -u "$f" ] || [ -g "$f" ]; } && sug="$sug $f"; done
[ -z "$sug" ] && bm_pass mounts.no-setuid "no setuid/setgid files in /bin /lib /lib64 /etc /init /agent" || bm_fail mounts.no-setuid "setuid/setgid files:$sug"
# Own-namespace facts.
for t in pid mnt net user ipc uts cgroup; do bm_info "mounts.ns:$t" "$(readlink /proc/self/ns/$t 2>&1)"; done
bm_info mounts.max_user_namespaces "$(while read -r l; do echo "$l"; done </proc/sys/user/max_user_namespaces 2>&1)"
bm_denied mounts.write-uid_map bash -c 'echo "0 1000 1" > /proc/self/uid_map'
bm_denied mounts.write-hostname bash -c 'echo pwned > /proc/sys/kernel/hostname'
m=""; while read -r _ mp _ opts _; do case "$mp" in /|/tmp|/home/learner|/proc|/sys|/dev) m="$m $mp($opts)";; esac; done </proc/mounts
bm_info mounts.table "$m"
case "$m" in */\(ro,*) bm_pass mounts.root-ro "root mounted ro";; *) bm_fail mounts.root-ro "root not mounted ro:$m";; esac
bm_done
