# Device nodes: nothing that reaches the host, hardware or kernel memory may be openable by the learner.
for n in kvm mem kmem port kmsg vhost-vsock vhost-net fuse loop-control watchdog rtc0 vda vda1 tpm0 ttyS0 hpet; do
  bm_cannot_open_r "devices.read:$n" "/dev/$n"
  bm_cannot_open_w "devices.write:$n" "/dev/$n"
done
bm_cannot_open_r devices.proc-kcore /proc/kcore
bm_cannot_open_r devices.proc-kmsg /proc/kmsg
bm_cannot_open_w devices.proc-sysrq /proc/sysrq-trigger
# /dev/vsock may exist (vsock is built in). Opening it is harmless; reaching the host through it is tested by bm-probe.
[ -e /dev/vsock ] && bm_info devices.vsock-node "present: $(ls -l /dev/vsock)" || bm_info devices.vsock-node "absent"
# Inventory of every node in /dev that the learner can open for writing, for the record.
w=""; for f in /dev/*; do [ -c "$f" ] || [ -b "$f" ] || continue; [ -w "$f" ] && w="$w ${f#/dev/}"; done
bm_info devices.world-writable "$w"
case " $w " in *" kvm "*|*" mem "*|*" kmem "*|*" port "*|*" vda "*) bm_fail devices.dangerous-writable "dangerous node writable:$w";; *) bm_pass devices.dangerous-writable "none of kvm/mem/kmem/port/vda writable";; esac
bm_done
