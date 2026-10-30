# /proc/sys and /sys: nothing writable by the learner, and nothing about the HOST in what the guest can read.
nw=0; list=""
for f in /proc/sys/*/* /proc/sys/*/*/* /proc/sys/*/*/*/* /sys/kernel/* /sys/kernel/*/* /sys/module/*/parameters/* /sys/class/*/* /sys/devices/virtual/*/* ; do
  [ -f "$f" ] && [ -w "$f" ] && { nw=$((nw + 1)); list="$list $f"; }
done
[ "$nw" -eq 0 ] && bm_pass procsys.nothing-writable "no writable file under /proc/sys or /sys (sampled)" || bm_fail procsys.nothing-writable "$nw writable:$list"
bm_cannot_open_w procsys.core_pattern /proc/sys/kernel/core_pattern
bm_cannot_open_w procsys.modprobe /proc/sys/kernel/modprobe
bm_cannot_open_w procsys.sysrq /proc/sys/kernel/sysrq
bm_cannot_open_w procsys.drop_caches /proc/sys/vm/drop_caches
bm_cannot_open_w procsys.uevent_helper /sys/kernel/uevent_helper
bm_cannot_open_r procsys.proc-1-mem /proc/1/mem
bm_cannot_open_r procsys.proc-1-environ /proc/1/environ
bm_cannot_open_r procsys.proc-kallsyms-addresses /proc/kcore
# Host-information leakage: the guest's view of the machine. INFO lines are for the record; markers of the host fail.
first() { local l; read -r l <"$1" 2>/dev/null; echo "$l"; }
bm_info procsys.version "$(first /proc/version)"
bm_info procsys.cmdline "$(first /proc/cmdline)"
bm_info procsys.hostname "$(first /proc/sys/kernel/hostname)"
cpu=""; while read -r k _ v; do [ "$k" = model ] && { cpu=$v; break; }; done </proc/cpuinfo; bm_info procsys.cpu-model "$cpu"
bm_info procsys.sys-hypervisor "$(ls /sys/hypervisor 2>&1 | head -n 3)"
bm_info procsys.dmi "$(ls /sys/class/dmi/id 2>&1 | head -n 3)"
bm_info procsys.block "$(ls /sys/block 2>&1)"
bm_info procsys.virtio "$(ls /sys/bus/virtio/devices 2>&1)"
bm_info procsys.net "$(ls /sys/class/net 2>&1)"
leak=""
for f in /proc/cmdline /proc/mounts /proc/self/environ /proc/self/mountinfo; do
  while read -r l; do
    case "$l" in *baremetal*|*jailer*|*/srv/*|*firecracker*|*secret*|*token*|*password*|*AUTH_*|*MONGODB*|*ANTHROPIC*) leak="$leak [$f: ${l:0:80}]";; esac
  done <"$f" 2>/dev/null
done
[ -z "$leak" ] && bm_pass procsys.no-host-markers "no host paths or secret-looking strings in cmdline, mounts, environ" || bm_fail procsys.no-host-markers "$leak"
bm_info procsys.environ "$(env | sort | head -n 20 | while read -r l; do printf '%s;' "$l"; done)"
bm_done
