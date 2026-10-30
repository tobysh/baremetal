# Syscall-level containment probes need a compiled helper (the guest has no compiler). bm-probe is built on the
# disposable host by ../probe/add-probe.sh and injected into a COPY of the rootfs. Without it these checks SKIP.
if [ -x /bin/bm-probe ]; then /bin/bm-probe; else bm_skip probe.helper "/bin/bm-probe is not in this rootfs (see ../probe/add-probe.sh)"; fi
bm_done
