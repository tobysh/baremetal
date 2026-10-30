# Classic shell fork bomb as the learner. Expect: RLIMIT_NPROC (256, set by the agent) caps the guest's processes;
# the host never sees guest processes (only the VMM threads), so host pids/memory/CPU stay inside the cgroup.
bm_info forkbomb.limits "uid=$(id -u) ulimit_u=$(ulimit -u) procs_before=$(bm_nprocs)"
[ "$(ulimit -u)" = 256 ] && bm_pass forkbomb.nproc-limit "RLIMIT_NPROC=256" || bm_fail forkbomb.nproc-limit "ulimit -u is $(ulimit -u), the agent should set 256"
ulimit -u unlimited 2>/dev/null && bm_fail forkbomb.raise-hard-limit "learner raised its own hard limit" || bm_pass forkbomb.raise-hard-limit "cannot raise RLIMIT_NPROC"
bomb() { bomb | bomb & }
bomb &
bm_sleep 8
n=$(bm_nprocs)
bm_info forkbomb.procs "procs_after_8s=$n"
[ "$n" -le 330 ] && bm_pass forkbomb.contained "guest process count $n stayed near the limit" || bm_fail forkbomb.contained "guest has $n processes; the limit is not holding"
bm_done
bm_sleep 3600
