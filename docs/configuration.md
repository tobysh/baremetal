# Configuration reference

All configuration is through environment variables. In development put them in `.env.local` (git-ignored). In production use a root-owned, mode-0600 environment file read by systemd; see [deployment-hardening.md](deployment-hardening.md). Defaults below were read from the source; where a variable has a function that owns its default, the file is named so you can check.

## Core (development minimum)

| Variable | Default | Meaning |
|---|---|---|
| `MONGODB_URI` | `mongodb://127.0.0.1:27017` | Connection string (`src/lib/db.ts`) |
| `MONGODB_DB` | `baremetal` | Database name |
| `AUTH_SECRET` | none | 32+ characters. Keys the HMACs and the encrypted email queue. Required; `openssl rand -base64 48`. Not stored in the database, so back it up separately |

## Production-required

`src/instrumentation.ts` refuses to boot when `NODE_ENV=production` unless these are valid, and lists every problem at once. It does not run during `next build`.

| Variable | Meaning |
|---|---|
| `NEXT_PUBLIC_SITE_URL` | Real `https://` origin. Inlined at build time, so set it for `npm run build` too. Used for canonical URLs, the sitemap, og:image and the same-origin check |
| `TRUSTED_PROXY_HOPS` | How many proxy hops to trust in `X-Forwarded-For` (usually `1`). See "Proxy contract" in the deployment guide |
| `RUNNER_SOCKET` | Absolute Unix socket path (≤ 100 bytes) of the runner service. Production never executes learner code in the web process |
| `SMTP_URL`, `EMAIL_FROM` | Outgoing mail for verification, recovery and admin-sent links. `SMTP_URL` is `smtp://user:pass@host:587` (STARTTLS) or `smtps://user:pass@host:465`, with special characters in the password percent-encoded. `EMAIL_FROM` is `"Baremetal <no-reply@your-domain>"` on a domain the provider has verified (set SPF, DKIM, DMARC). Unset in development, mail is printed to the server console; production refuses to boot without both. The admin overview shows whether SMTP is configured and how deep the retry queue is |
| `LEGAL_ENTITY_NAME`, `LEGAL_ADDRESS`, `LEGAL_CONTACT_EMAIL` | Shown in the footer and legal pages. Optional: `LEGAL_JURISDICTION` (default "England and Wales") |

## Tutor

| Variable | Default | Meaning |
|---|---|---|
| `ANTHROPIC_API_KEY` | none | Enables the Anthropic provider |
| `TUTOR_PROVIDER` | `anthropic` | `ollama` switches to a local model |
| `TUTOR_MODEL` | `claude-haiku-5-5` | Claude model id |
| `OLLAMA_HOST`, `OLLAMA_MODEL` | `http://127.0.0.1:11434`, `qwen2.5-coder:7b` | Ollama endpoint and model |
| `OLLAMA_TIMEOUT_MS` | `120000` | Deadline for one Ollama tutor call |
| `TUTOR_BUDGET_USER_DAY_USD` | `0.25` | Per-user daily spend ceiling |
| `TUTOR_BUDGET_GLOBAL_DAY_USD` | `20` | Site-wide daily spend ceiling |
| `TUTOR_PRICE_IN_PER_MTOK`, `TUTOR_PRICE_OUT_PER_MTOK` | `1`, `5` | USD per million tokens used to estimate cost |
| `TUTOR_PRICE_CACHE_READ_MULT`, `TUTOR_PRICE_CACHE_WRITE_MULT` | `0.1`, `1.25` | Cache pricing as multiples of the input price |
| `TUTOR_LIMIT_USER_HOUR`, `TUTOR_LIMIT_USER_DAY` | `20`, `100` | Questions per account |
| `TUTOR_LIMIT_IP_HOUR` | `120` | Per IP, **accounts under 24 h old only** |
| `TUTOR_LIMIT_GLOBAL_HOUR` | `2000` | Whole site |
| `TUTOR_LIMIT_CACHE_HIT_IP_HOUR` | `300` | Cached answers per IP (cheap, but scrapeable) |

## Rate limits (web app)

Per-IP limits are deliberately generous because schools and offices share addresses; signed-in traffic is limited per account. Details and rationale: "Per-IP limits" in the deployment guide.

| Variable | Default |
|---|---|
| `AUTH_LIMIT_SIGNUP_IP` | 30 per 15 min |
| `AUTH_LIMIT_LOGIN_IP` | 60 per 15 min |
| `CAPTCHA_LIMIT_IP_MINUTE` | 120 per min |
| `RUN_LIMIT_ANON_IP` | 30 per 10 min (anonymous runs) |

## Runner service

| Variable | Default | Meaning |
|---|---|---|
| `RUNNER_SOCKET` | none | Socket the service listens on (also required by the web app) |
| `RUNNER_COMPILE_SLOTS`, `RUNNER_RUN_SLOTS` | `2`, `3` | Concurrent compiles / program runs. Size against the unit's `MemoryMax` |
| `RUNNER_COMPILE_BUDGET` | `60` | Compiles per account or IP per minute, each `set`-case recompile counting as one |
| `RUNNER_INFLIGHT_USER`, `RUNNER_INFLIGHT_IP` | `2`, `6` | Queued + running requests per account / per anonymous IP |
| `RUNNER_REQUEST_BYTES`, `RUNNER_RESPONSE_BYTES`, `RUNNER_REQUEST_TIMEOUT_MS` | see `runner-protocol.ts` | Wire-protocol bounds |
| `RUNNER_CGROUP_ROOT` | unset | Delegated cgroup-v2 subtree for per-job limits. **Required for the production runner** |
| `GO_CACHE_SEED` | unset | Directory holding a prewarmed Go std cache (`scripts/warm-go-cache.mts`); needs bubblewrap ≥ 0.10 |
| `GO_TOOLCHAIN_DIR`, `RUST_TOOLCHAIN_DIR` | auto-detected | Override toolchain locations |

## Interactive shell (optional)

Set on the broker/launcher units; names and status are in [shell-deployment.md](shell-deployment.md).

| Variable | Default | Meaning |
|---|---|---|
| `SHELL_BROKER_SOCKET` | none | Broker socket; also read by the web app to find it |
| `SHELL_LAUNCHER_SOCKET` | none | Launcher socket the broker talks to |
| `SHELL_MAX_SESSIONS` | `4` | Concurrent sessions |
| `SHELL_IDLE_SECONDS`, `SHELL_MAX_SECONDS` | `300`, `1800` | Idle and absolute session limits |
| `SHELL_OUTPUT_PER_SEC`, `SHELL_OUTPUT_BURST`, `SHELL_OUTPUT_FLOOD_SECONDS` | 128 KiB, 512 KiB, `60` | Output rate limiting |
| `SHELL_KERNEL`, `SHELL_ROOTFS`, `SHELL_CHROOT_BASE`, `SHELL_BROKER_GID`, `SHELL_VM_UID_BASE`, `SHELL_MAX_VMS`, `SHELL_VM_MAX_SECONDS` | see `scripts/shell-launcher.mts` | Launcher inputs. Must be root-owned (`trusted-path.ts`) |
| `SHELL_MODE=unjailed` | unset | Development only; refused in production |

## Scripts and tooling

| Variable | Used by |
|---|---|
| `BACKUP_DIR` (default `~/baremetal-backups`), `BACKUP_KEEP` (14) | `npm run backup-db` / `restore-db` |
| `E2E_ORIGIN`, `TEST_BASE_URL`, `REGRESSION_PORT`, `LOAD_ORIGIN` | e2e, regression and load scripts |
| `PLAYWRIGHT_CHROMIUM_EXECUTABLE` | Use an installed Chromium for `npm run test:browser` |
| `BM_*` | Hostile-workload suite for the shell (`scripts/shell/hostile/README.md`) |
