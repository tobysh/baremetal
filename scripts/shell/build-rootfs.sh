#!/bin/sh
# Build a minimal guest rootfs (ext4) from host binaries. Usage: build-rootfs.sh <out.ext4>
# No root needed: mkfs.ext4 -d populates the image from a staging directory.
# Prototype image: learners get bash and a few coreutils, no compilers, no secrets.
set -eu
out=${1:?usage: build-rootfs.sh <out.ext4>}
stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT
mkdir -p "$stage"/bin "$stage"/lib "$stage"/lib64 "$stage"/proc "$stage"/sys "$stage"/dev "$stage"/tmp "$stage"/home/learner "$stage"/etc
bins="setpriv bash ls cat echo id ps mount sleep uname head grep sed mkdir rm touch pwd env whoami date wc sort"
for b in $bins; do
  src=/usr/bin/$b
  [ -x "$src" ] || { echo "missing host binary: $src" >&2; exit 1; }
  cp "$src" "$stage/bin/$b"
  ldd "$src" 2>/dev/null | grep -o '/[^ ]*' | while read -r lib; do
    [ -f "$lib" ] && mkdir -p "$stage$(dirname "$lib")" && cp -n "$lib" "$stage$lib"
  done
done
ln -sf bash "$stage/bin/sh"
cat > "$stage/etc/passwd" <<P
root:x:0:0:root:/root:/bin/bash
learner:x:1000:1000:learner:/home/learner:/bin/bash
P
echo "learner:x:1000:" > "$stage/etc/group"
cat > "$stage/init" <<'I'
#!/bin/bash
export PATH=/bin
mount -t proc proc /proc
mount -t sysfs sys /sys
mount -t devtmpfs dev /dev 2>/dev/null
mount -t tmpfs -o size=32m tmpfs /tmp
mount -t tmpfs -o size=32m,uid=1000,gid=1000 tmpfs /home/learner
echo "== baremetal shell prototype (type 'exit' to end) =="
# The shell runs unprivileged with no capabilities and no_new_privs; init stays root only to power off.
cd /home/learner
HOME=/home/learner setpriv --reuid 1000 --regid 1000 --clear-groups --bounding-set -all --no-new-privs \
  bash --noprofile --norc -i
# Init exiting panics the guest; panic=1 + reboot=k then makes Firecracker exit (sysrq poweroff/reboot do not).
exit 0
I
chmod 755 "$stage/init"
chmod 1777 "$stage/tmp"
rm -f "$out"
truncate -s 128M "$out"
mkfs.ext4 -q -F -d "$stage" "$out"
echo "wrote $out"
