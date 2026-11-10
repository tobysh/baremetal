# Capacity

Rough numbers for a single runner host. Measure again on the real host before relying on them.

## Resource enforcement validation

Run `node --import tsx scripts/runner/host-checks.mts --disposable` as root only
on an otherwise idle disposable deployment host. It requires the installed runner
service, root-owned mode-0600 web environment and a reachable host MongoDB, then
checks that the runner UID cannot reach either the web secrets or database.
Bounded native probes verify OOM enforcement, fork rejection with a recorded
`pids.events` limit event, CPU throttling counters, and cancellation of a learner
parent plus a `setsid` child and grandchild. It checks that observed descendants
stop before the ordinary execution timeout, all job cgroups disappear and a
subsequent healthy job succeeds. Requests and polling have explicit deadlines.

The CPU probe needs at least three effective CPUs to exceed the configured
two-CPU job quota. A smaller host, a more restrictive ancestor quota or heavy
contention cannot establish that leaf's throttling; a failed check must be
reported as missing validation evidence, not a pass. This script has not yet
been exercised on the intended deployment host. Record the host/kernel/systemd
configuration and complete output before treating isolation as production-verified.

## Model (from the code)

One runner process, two compile slots and three run slots (`RUNNER_COMPILE_SLOTS`, `RUNNER_RUN_SLOTS`), up to 24 waiting jobs per pool, 30 s queue timeout. A job holds a compile slot only while its compiler runs. Memory arithmetic: `docs/deployment-hardening.md`, "Runner slots and memory".

## Load test

`npm run load:run-mix -- --users 5 --seconds 60` (see `scripts/load/run-mix.mts`; it refuses non-localhost URLs unless you pass `--i-own-this-host`, and anonymous traffic needs a raised `RUN_LIMIT_ANON_IP`) against a local production build with a separate runner service. Mix 50% C, 25% Rust, 15% Go, 10% assembly, Test mode with the reference solutions, five users each with their own address, no Go cache seed.

Run on 30 October 2026, development machine, `RUN_LIMIT_ANON_IP` raised so the anonymous per-IP limit does not interfere:

| language | requests | p50 ms | p95 ms | 503 | errors |
|---|---|---|---|---|---|
| C | 109 | 511 | 2875 | 0 | 0 |
| Rust | 49 | 644 | 3044 | 0 | 0 |
| Go | 21 | 5304 | 7556 | 0 | 0 |
| Assembly | 22 | 1271 | 5608 | 0 | 0 |

201 requests in 60 s (about 3.4 per second) with no busy responses, errors or wrong verdicts. Go dominates latency because every compile rebuilds the standard library; with `GO_CACHE_SEED` a single Go Test dropped from about 3.5 s to 0.3 s (`docs/deployment-hardening.md`, "Go build cache seed"), so repeat the run with the seed once the host supports overlays.
