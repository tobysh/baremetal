#!/bin/sh
# Prototype: boot one throwaway microVM under the Firecracker jailer, console on this terminal.
# Usage: sudo launch-jailed.sh <vmlinux> <rootfs.ext4> [max-seconds]
# Needs root for chroot, uid switch and cgroups. Run on a disposable machine for hostile tests.
set -eu
[ "$(id -u)" = 0 ] || { echo "run as root (sudo)" >&2; exit 1; }
kernel=${1:?kernel} rootfs=${2:?rootfs} max=${3:-120}
# Use the official static release binaries (FIRECRACKER / JAILER). A distro build is dynamically linked and cannot
# exec inside the empty jailer chroot ("Failed to exec into Firecracker: No such file or directory").
fc=${FIRECRACKER:-$(command -v firecracker)} jailer=${JAILER:-$(command -v jailer)}
[ -z "$(file -L "$fc" | grep -i 'dynamically linked')" ] || { echo "$fc is dynamically linked; set FIRECRACKER to a static release binary" >&2; exit 1; }
base=${JAILER_BASE:-/srv/baremetal-jailer}
uid=${JAILER_UID:-300001}   # dedicated unprivileged uid/gid for the VMM, not a real account
id=bm-$(od -An -N6 -tx1 /dev/urandom | tr -d ' \n')
name=$(basename "$fc")   # the jailer names its chroot and cgroup after the exec file
root=$base/$name/$id/root
mkdir -p "$base"
cleanup() {
  # Kill everything in the VM's cgroup (covers the new pid namespace), then remove the chroot and cgroup, which the jailer leaves behind.
  echo 1 > "/sys/fs/cgroup/$name/$id/cgroup.kill" 2>/dev/null || true
  sleep 0.3
  rm -rf "$base/$name/$id"
  rmdir "/sys/fs/cgroup/$name/$id" 2>/dev/null || true
}
trap cleanup EXIT INT TERM
# Create the chroot ourselves so the guest files are in place before the jailer drops privileges.
mkdir -p "$root"
cp --reflink=auto "$kernel" "$root/vmlinux"
cp --reflink=auto "$rootfs" "$root/rootfs.ext4"   # per-session copy: nothing the guest does can change the shared image
cat > "$root/vm.json" <<J
{
  "boot-source": { "kernel_image_path": "/vmlinux", "boot_args": "console=ttyS0 quiet loglevel=0 reboot=k panic=1 pci=off ro init=/init bm.serial=1 random.trust_cpu=on" },
  "drives": [ { "drive_id": "rootfs", "path_on_host": "/rootfs.ext4", "is_root_device": true, "is_read_only": true } ],
  "machine-config": { "vcpu_count": 1, "mem_size_mib": 256, "smt": false },
  "network-interfaces": []
}
J
chown -R "$uid:$uid" "$base/$name/$id"
# With --new-pid-ns the jailer forks and its parent exits at once, so its exit does not mean the VM is done.
# The VM's cgroup is the handle: wait until it is empty (guest exited) or the wall-clock cap passes; cleanup() kills the rest.
"$jailer" --id "$id" --exec-file "$fc" --uid "$uid" --gid "$uid" \
  --chroot-base-dir "$base" --cgroup-version 2 --new-pid-ns \
  --cgroup cpu.max="50000 100000" --cgroup memory.max=400M --cgroup pids.max=64 \
  -- --no-api --config-file /vm.json --level Error
cg=/sys/fs/cgroup/$name/$id
alive() { [ -n "$(cat "$cg/cgroup.procs" 2>/dev/null)" ]; }   # cgroup files report size 0, so test the contents
deadline=$(( $(date +%s) + max ))
sleep 0.5
while alive && [ "$(date +%s)" -lt "$deadline" ]; do sleep 0.2; done
if alive; then echo "[session cap of ${max}s reached; killing VM]" >&2; fi
exit 0
