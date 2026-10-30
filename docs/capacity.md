# Capacity

Rough numbers for a single runner host. Measure again on the real host before relying on them.

## Model (from the code)

One runner process, two compile slots and three run slots (`RUNNER_COMPILE_SLOTS`, `RUNNER_RUN_SLOTS`), up to 24 waiting jobs per pool, 30 s queue timeout. A job holds a compile slot only while its compiler runs. Memory arithmetic: `docs/deployment-hardening.md`, "Runner slots and memory".

## Load test

`npm run load:run-mix -- --users 5 --seconds 60` (see `scripts/load/run-mix.mts`; README, "Useful commands") against a local production build with a separate runner service. Mix 50% C, 25% Rust, 15% Go, 10% assembly, Test mode with the reference solutions, five users each with their own address, no Go cache seed.

Run on 30 October 2026, development machine, `RUN_LIMIT_ANON_IP` raised so the anonymous per-IP limit does not interfere:

| language | requests | p50 ms | p95 ms | 503 | errors |
|---|---|---|---|---|---|
| C | 109 | 511 | 2875 | 0 | 0 |
| Rust | 49 | 644 | 3044 | 0 | 0 |
| Go | 21 | 5304 | 7556 | 0 | 0 |
| Assembly | 22 | 1271 | 5608 | 0 | 0 |

201 requests in 60 s (about 3.4 per second) with no busy responses, errors or wrong verdicts. Go dominates latency because every compile rebuilds the standard library; with `GO_CACHE_SEED` a single Go Test dropped from about 3.5 s to 0.3 s (`docs/deployment-hardening.md`, "Go build cache seed"), so repeat the run with the seed once the host supports overlays.
