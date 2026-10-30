# Cross-process access to the root-owned agent and init (PID 1 is /init -> /agent, running as root).
# bash has no ptrace(2); /proc/<pid>/mem needs the same PTRACE_MODE_ATTACH check. Real PTRACE_ATTACH is in bm-probe.
agent=""; for p in /proc/[0-9]*; do read -r c <"$p/comm" 2>/dev/null; [ "$c" = agent ] && agent=${p#/proc/}; done
bm_info ptrace.agent-pid "${agent:-not found}; pid1 comm=$(first=; read -r first </proc/1/comm; echo "$first")"
for pid in 1 ${agent:+$agent}; do
  bm_cannot_open_r "ptrace.mem:$pid" "/proc/$pid/mem"
  bm_cannot_open_w "ptrace.mem-w:$pid" "/proc/$pid/mem"
  bm_cannot_open_r "ptrace.environ:$pid" "/proc/$pid/environ"
  bm_cannot_open_r "ptrace.syscall:$pid" "/proc/$pid/syscall"
  bm_cannot_open_r "ptrace.stack:$pid" "/proc/$pid/stack"
  bm_denied "ptrace.root-link:$pid" ls "/proc/$pid/root/"
  bm_denied "ptrace.cwd-link:$pid" ls "/proc/$pid/cwd/"
  bm_denied "ptrace.fd:$pid" ls "/proc/$pid/fd/"
  bm_denied "ptrace.ns-link:$pid" readlink "/proc/$pid/ns/mnt"
done
read -r scope </proc/sys/kernel/yama/ptrace_scope 2>/dev/null; bm_info ptrace.yama_scope "${scope:-yama absent}"
bm_info ptrace.dumpable "$(while read -r l; do case "$l" in Seccomp*|NoNewPrivs*|CapEff*) printf '%s; ' "$l";; esac; done </proc/self/status)"
bm_done
