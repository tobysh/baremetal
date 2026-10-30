# Signalling PID 1 and the root-owned agent. All must be refused, and both must still be alive afterwards.
agent=""; for p in /proc/[0-9]*; do read -r c <"$p/comm" 2>/dev/null; [ "$c" = agent ] && agent=${p#/proc/}; done
for sig in 0 TERM INT HUP QUIT KILL STOP CONT USR1 USR2; do
  bm_denied "signal.pid1:$sig" kill -s "$sig" 1
  [ -n "$agent" ] && bm_denied "signal.agent:$sig" kill -s "$sig" "$agent"
done
# kill builtin as well as the external path (same syscall, but both exit statuses are checked).
kill -9 1 2>/dev/null && bm_fail signal.builtin-kill-1 "kill -9 1 returned success" || bm_pass signal.builtin-kill-1 "kill -9 1 refused"
[ -d /proc/1 ] && bm_pass signal.pid1-alive "init still present" || bm_fail signal.pid1-alive "/proc/1 gone"
[ -z "$agent" ] || { [ -d "/proc/$agent" ] && bm_pass signal.agent-alive "agent $agent still present" || bm_fail signal.agent-alive "agent $agent gone"; }
bm_info signal.agent-pid "${agent:-not found}"
bm_done
