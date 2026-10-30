#!/bin/sh
# Prototype: boot one throwaway microVM and attach its serial console to this terminal.
# Usage: launch.sh <vmlinux> <rootfs.ext4> [max-seconds]
# Unjailed: use only on a disposable machine for anything hostile. The jailed variant is launch-jailed.sh.
set -eu
kernel=${1:?kernel} rootfs=${2:?rootfs} max=${3:-120}
dir=$(mktemp -d)
trap 'rm -rf "$dir"' EXIT
cat > "$dir/vm.json" <<J
{
  "boot-source": { "kernel_image_path": "$kernel", "boot_args": "console=ttyS0 quiet loglevel=0 reboot=k panic=1 pci=off ro init=/init bm.serial=1 random.trust_cpu=on" },
  "drives": [ { "drive_id": "rootfs", "path_on_host": "$rootfs", "is_root_device": true, "is_read_only": true } ],
  "machine-config": { "vcpu_count": 1, "mem_size_mib": 256, "smt": false },
  "network-interfaces": []
}
J
# --no-api: the VM is configured once from the file and cannot be reconfigured at runtime.
timeout --signal=KILL "$max" firecracker --no-api --config-file "$dir/vm.json" --level Error
