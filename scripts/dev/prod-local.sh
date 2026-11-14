#!/usr/bin/env bash
# Runs the app in PRODUCTION mode locally and runs the e2e + API hardening suites against it.
#
#   scripts/dev/prod-local.sh [--no-build] [--keep] [--port 3500]
#
# Builds with NEXT_PUBLIC_SITE_URL=https://localhost:<port> (inlined at build time; production only compares the
# Origin *host*, so http://localhost:<port> test requests pass the same-origin check), starts the runner service on
# a private socket and `next start` with every production-required variable, runs the suites, then kills what it
# started (by PID). Uses its own database (MONGODB_DB, default bm_prod_local) on the mongod from `npm run db`, which
# must already be running. SMTP_URL points at a dummy port that never delivers. Never touches `baremetal`.
# The test clients replay cookies by name and ignore Secure, so BM_INSECURE_COOKIES is not needed. Note that
# `next build` writes ./.next, so do not run it while `npm run dev` is using this checkout.
set -euo pipefail
cd "$(dirname "$0")/../.."
PORT=3500; BUILD=1; KEEP=0
while [ $# -gt 0 ]; do
  case "$1" in
    --no-build) BUILD=0 ;; --keep) KEEP=1 ;; --port) PORT="$2"; shift ;;
    *) echo "unknown option $1" >&2; exit 2 ;;
  esac; shift
done
WORK="$(mktemp -d "${TMPDIR:-/tmp}/bm-prod-local.XXXXXX")"
SOCK="$WORK/runner.sock"
PIDS=()
cleanup() {
  for p in "${PIDS[@]:-}"; do [ -n "$p" ] && kill "$p" 2>/dev/null || true; done
  sleep 1
  for p in "${PIDS[@]:-}"; do [ -n "$p" ] && kill -9 "$p" 2>/dev/null || true; done
  [ "$KEEP" = 1 ] || rm -rf "$WORK"
}
trap cleanup EXIT

export MONGODB_URI="${MONGODB_URI:-mongodb://127.0.0.1:27017}"
export MONGODB_DB="${MONGODB_DB:-bm_prod_local}"
[ "$MONGODB_DB" != baremetal ] || { echo "refusing to use the dev database" >&2; exit 2; }
export NEXT_PUBLIC_SITE_URL="https://localhost:$PORT"
export AUTH_SECRET="${AUTH_SECRET:-$(head -c 48 /dev/urandom | base64 | tr -d '\n=')}"
export TRUSTED_PROXY_HOPS=1
export LEGAL_ENTITY_NAME="Local Test Ltd" LEGAL_ADDRESS="1 Test Street, Testville" LEGAL_CONTACT_EMAIL="legal@example.test"
export SMTP_URL="smtp://u:p@127.0.0.1:9/" EMAIL_FROM="Baremetal <no-reply@example.test>"
export RUNNER_SOCKET="$SOCK"
export E2E_ORIGIN="https://localhost:$PORT"

(echo > /dev/tcp/127.0.0.1/27017) 2>/dev/null || { echo "mongod is not running on 27017 (npm run db)" >&2; exit 1; }
if [ "$BUILD" = 1 ]; then NODE_ENV=production npx next build; fi

# The runner must not see web secrets, and needs no production flag locally (no delegated cgroup here).
env -u AUTH_SECRET -u MONGODB_URI -u SMTP_URL -u ANTHROPIC_API_KEY npx tsx scripts/runner-service.mts >"$WORK/runner.log" 2>&1 &
PIDS+=($!)
for _ in $(seq 60); do [ -S "$SOCK" ] && break; sleep 0.5; done
[ -S "$SOCK" ] || { cat "$WORK/runner.log"; echo "runner did not start" >&2; exit 1; }

NODE_ENV=production npx next start -p "$PORT" >"$WORK/web.log" 2>&1 &
PIDS+=($!)
for _ in $(seq 60); do curl -fs -o /dev/null "http://localhost:$PORT/login" && break; sleep 0.5; done
curl -fs -o /dev/null "http://localhost:$PORT/login" || { cat "$WORK/web.log"; echo "web did not start" >&2; exit 1; }
echo "== boot log"; grep -E "\[boot\]|\[xp\]|Error" "$WORK/web.log" || true

rc=0
echo "== test:e2e"
npm run test:e2e -- "http://localhost:$PORT" || rc=1
echo "== api-hardening"
node --conditions=react-server --import tsx scripts/security/api-hardening.mts "http://localhost:$PORT" || rc=1
echo "== web log tail"; tail -n 30 "$WORK/web.log"
[ "$KEEP" = 1 ] && echo "logs kept in $WORK"
exit $rc
