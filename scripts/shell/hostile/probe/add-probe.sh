#!/bin/sh
# Build bm-probe and inject it into a COPY of the guest rootfs (needs gcc and e2fsprogs' debugfs; no root, no mount).
# Usage: add-probe.sh <base-rootfs.ext4> <out-rootfs.ext4>
# Then run the suite with SHELL_ROOTFS=<out-rootfs.ext4>. Never use the probe image in production.
set -eu
base=${1:?usage: add-probe.sh <base-rootfs.ext4> <out-rootfs.ext4>}
out=${2:?usage: add-probe.sh <base-rootfs.ext4> <out-rootfs.ext4>}
here=$(cd "$(dirname "$0")" && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
gcc -static -O2 -Wall -o "$tmp/bm-probe" "$here/bm-probe.c"
cp "$base" "$out"
debugfs -w -R "write $tmp/bm-probe /bin/bm-probe" "$out" >/dev/null
debugfs -w -R "set_inode_field /bin/bm-probe mode 0100755" "$out" >/dev/null
debugfs -w -R "set_inode_field /bin/bm-probe uid 0" "$out" >/dev/null
debugfs -w -R "set_inode_field /bin/bm-probe gid 0" "$out" >/dev/null
echo "wrote $out (contains /bin/bm-probe)"
