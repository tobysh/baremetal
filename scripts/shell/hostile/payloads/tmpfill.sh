# Fill the writable tmpfs mounts (/tmp and the home dir, 32 MiB each) and the inode table; try the read-only root.
fill() { # fill dir: append 64 KiB at a time until the write fails
  while printf '%065535d\n' 0 >> "$1/bm-fill" 2>/dev/null; do :; done
  local sz=0 line; line=$(ls -l "$1/bm-fill" 2>/dev/null); set -- $line; sz=${5:-0}; echo "$sz"
}
for d in /tmp /home/learner; do
  sz=$(fill "$d")
  bm_info "tmpfill.$d" "stopped at ${sz} bytes"
  [ "$sz" -le $((40 * 1024 * 1024)) ] && bm_pass "tmpfill.size-bounded:$d" "${sz} bytes <= 40 MiB" || bm_fail "tmpfill.size-bounded:$d" "${sz} bytes written; tmpfs size= is not holding"
  rm -f "$d/bm-fill"
done
bm_cannot_open_w tmpfill.rootfs-readonly /etc/passwd
bm_denied tmpfill.rootfs-create bash -c ': > /bm-new'
# Inodes: tmpfs inodes cost kernel memory that size= does not count. Bounded here; the host-side memory check is authoritative.
mkdir -p /tmp/ino; i=0
while [ "$i" -lt 150000 ] && : > "/tmp/ino/f$i" 2>/dev/null; do i=$((i + 1)); done
bm_info tmpfill.inodes "created $i empty files; MemAvailable_kB=$(bm_meminfo MemAvailable)"
rm -rf /tmp/ino
bm_done
