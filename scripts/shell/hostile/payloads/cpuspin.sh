# Busy loops. Guest has 1 vCPU; the host jailer cgroup limits the VMM to cpu.max (default 50000/100000 = half a core).
# Expect: the host-side driver sees VM CPU time bounded by the quota while the loops run.
for _i in 1 2 3 4; do ( while :; do :; done ) & done
bm_info cpuspin.start "4 spinners, nprocs=$(bm_nprocs)"
bm_sleep 25
bm_done
bm_sleep 3600
