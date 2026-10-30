#!/bin/bash
# Hostile-workload suite for the microVM shell. DESTRUCTIVE BY DESIGN: fork bombs, memory exhaustion, disk fill,
# CPU spin and output floods run inside real microVMs, with the host-side limits as the only protection.
# Run ONLY on a disposable VM that holds nothing you care about, as root. See README.md.
#
#   BM_DISPOSABLE_HOST=yes-this-machine-is-disposable \
#   FIRECRACKER=... JAILER=... SHELL_KERNEL=... SHELL_ROOTFS=... ./run.sh [driver options]
#
# --list and --dry-run are harmless (nothing starts) and need neither root nor the guard.
set -u
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)

for a in "$@"; do
  case "$a" in --list|--dry-run) cd "$root" && exec "$root/node_modules/.bin/tsx" "$here/driver.mts" "$@";; esac
done

GUARD=yes-this-machine-is-disposable
cat >&2 <<'W'
############################################################################
#  WARNING: HOSTILE WORKLOAD TESTS                                         #
#  These run fork bombs, memory bombs, disk fills, CPU spins, output       #
#  floods and containment probes in microVMs on THIS machine.              #
#  A bug in the limits can hang or crash the host. Only run this on a      #
#  disposable VM with no data, no secrets and no other services.           #
############################################################################
W
[ "${BM_DISPOSABLE_HOST:-}" = "$GUARD" ] || { echo "REFUSING: set BM_DISPOSABLE_HOST=$GUARD to confirm this machine is disposable." >&2; exit 2; }
[ "$(id -u)" = 0 ] || { echo "REFUSING: must run as root (the jailer needs it)." >&2; exit 2; }
[ "$(uname -s)" = Linux ] || { echo "REFUSING: Linux only." >&2; exit 2; }
for v in FIRECRACKER JAILER SHELL_KERNEL SHELL_ROOTFS; do
  eval "val=\${$v:-}"
  [ -n "$val" ] || { echo "REFUSING: $v is not set (see README.md)." >&2; exit 2; }
  [ -e "$val" ] || { echo "REFUSING: $v=$val does not exist." >&2; exit 2; }
done
# The distro firecracker is dynamically linked and cannot exec inside the empty jailer chroot.
if file -L "$FIRECRACKER" | grep -qi 'dynamically linked'; then echo "REFUSING: $FIRECRACKER is dynamically linked; use the static release binary." >&2; exit 2; fi
# Soft sanity checks that this really is a throwaway box.
for p in mongod nginx apache2 postgres sshd-session; do
  pgrep -x "$p" >/dev/null 2>&1 && echo "NOTE: $p is running here. Is this really a disposable machine?" >&2
done
if [ -z "${BM_ASSUME_YES:-}" ] && [ -t 0 ]; then
  printf 'Type the hostname (%s) to continue: ' "$(hostname)" >&2
  read -r answer; [ "$answer" = "$(hostname)" ] || { echo "aborted." >&2; exit 2; }
fi

: "${BM_GLOBAL_TIMEOUT_S:=3600}"
: "${BM_RESULTS_DIR:=/var/tmp/bm-hostile}"
mkdir -p "$BM_RESULTS_DIR"
stamp=$(date +%Y%m%d-%H%M%S)
log=$BM_RESULTS_DIR/run-$stamp.log
export BM_RESULTS_JSON=$BM_RESULTS_DIR/run-$stamp.json BM_DISPOSABLE_HOST
export FIRECRACKER JAILER SHELL_KERNEL SHELL_ROOTFS BM_GLOBAL_TIMEOUT_S

# Sweep: kill every VM cgroup and remove chroots, whatever happened. Disposable host, so every bm-* is ours.
sweep() {
  local name cg base
  name=$(basename "$FIRECRACKER"); cg=${SHELL_CGROUP_ROOT:-/sys/fs/cgroup}/$name; base=${SHELL_CHROOT_BASE:-/srv/baremetal-jailer}/$name
  for d in "$cg"/bm-*; do [ -d "$d" ] && { echo 1 > "$d/cgroup.kill" 2>/dev/null; }; done
  sleep 1
  for d in "$cg"/bm-*; do [ -d "$d" ] && rmdir "$d" 2>/dev/null; done
  rm -rf "$base"/bm-* 2>/dev/null
}
trap 'echo "[run.sh] interrupted, sweeping VMs" >&2; sweep; exit 130' INT TERM

echo "[run.sh] log: $log  results json: $BM_RESULTS_JSON  global timeout: ${BM_GLOBAL_TIMEOUT_S}s" >&2
cd "$root" || exit 2
# Hard outer timeout (the driver also has its own). --kill-after makes sure a wedged driver is gone.
timeout --signal=TERM --kill-after=30 "$((BM_GLOBAL_TIMEOUT_S + 60))" "$root/node_modules/.bin/tsx" "$here/driver.mts" "$@" 2>&1 | tee "$log"
status=${PIPESTATUS[0]}
[ "$status" = 124 ] || [ "$status" = 137 ] && echo "[run.sh] OUTER TIMEOUT: the driver was killed" >&2
sweep
left=$(ls -d "${SHELL_CGROUP_ROOT:-/sys/fs/cgroup}/$(basename "$FIRECRACKER")"/bm-* 2>/dev/null | wc -l)
echo "[run.sh] exit status $status; VM cgroups left after sweep: $left; log $log" >&2
[ "$status" = 0 ] && [ "$left" = 0 ] || exit "$([ "$status" = 0 ] && echo 1 || echo "$status")"
exit 0
