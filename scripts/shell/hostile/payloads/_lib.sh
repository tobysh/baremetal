# Guest-side helpers. The driver prepends this file to every payload and runs the result with bash inside the guest.
# Only bash builtins and the coreutils subset in scripts/shell/build-rootfs.sh exist there: no compiler, no python.
# Result lines (parsed by driver.mts; a hostile guest can print anything, so the host-side assertions are what count):
#   BM_CHECK<TAB>name<TAB>PASS|FAIL|INFO|SKIP<TAB>detail          BM_DONE<TAB>42
bm_pass() { printf 'BM_CHECK\t%s\tPASS\t%s\n' "$1" "${2:-}"; }
bm_fail() { printf 'BM_CHECK\t%s\tFAIL\t%s\n' "$1" "${2:-}"; }
bm_info() { printf 'BM_CHECK\t%s\tINFO\t%s\n' "$1" "${2:-}"; }
bm_skip() { printf 'BM_CHECK\t%s\tSKIP\t%s\n' "$1" "${2:-}"; }
bm_done() { printf 'BM_DONE\t%s\n' "$((6 * 7))"; }   # computed so the source text never equals the output
# bm_denied name cmd...: PASS when the command fails (the operation was refused).
# Exit status 127 (tool not in the image) is a SKIP, not a pass: a missing binary proves nothing about the kernel.
bm_denied() { local n=$1 rc; shift; "$@" >/dev/null 2>&1; rc=$?; if [ "$rc" = 0 ]; then bm_fail "$n" "unexpectedly succeeded: $*"; elif [ "$rc" = 127 ]; then bm_skip "$n" "tool missing in image: $1"; else bm_pass "$n" "refused (rc=$rc): $*"; fi; }
# bm_cannot_open_r / _w name path: opening only (write opens use append, nothing is ever written).
bm_cannot_open_r() { if [ ! -e "$2" ]; then bm_pass "$1" "absent: $2"; elif ( exec 9<"$2" ) 2>/dev/null; then bm_fail "$1" "opened for reading: $2"; else bm_pass "$1" "not readable: $2"; fi; }
bm_cannot_open_w() { if [ ! -e "$2" ]; then bm_pass "$1" "absent: $2"; elif ( exec 9>>"$2" ) 2>/dev/null; then bm_fail "$1" "opened for writing: $2"; else bm_pass "$1" "not writable: $2"; fi; }
# Sleep without forking (a fork bomb makes fork fail): wait on a pipe fd opened up front.
exec 9<> <(:)
bm_sleep() { read -r -t "$1" -u 9 _ 2>/dev/null || true; }
# Count processes without forking.
bm_nprocs() { local d n=0; for d in /proc/[0-9]*; do n=$((n + 1)); done; echo "$n"; }
bm_meminfo() { local k v _; while read -r k v _; do [ "$k" = "$1:" ] && { echo "$v"; return; }; done </proc/meminfo; }
bm_has_cmd() { type -P "$1" >/dev/null 2>&1; }
