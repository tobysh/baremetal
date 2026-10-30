# Memory exhaustion with shell string doubling (no compiler needed). Guest RAM is 256 MiB; the host cgroup allows 400 MiB.
# Expect: the guest OOM killer (or the broker) ends it; the host cgroup never exceeds memory.max.
bm_info memhog.before "MemAvailable_kB=$(bm_meminfo MemAvailable)"
grow() { local s=0123456789abcdef; while :; do s=$s$s; done; }
( grow ) & ( grow ) & ( grow ) &
bm_sleep 25
bm_info memhog.after "MemAvailable_kB=$(bm_meminfo MemAvailable) procs=$(bm_nprocs)"
bm_done
bm_sleep 3600
