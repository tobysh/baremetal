# Deployment

This is the single place for everything that must happen before Baremetal is exposed publicly. Local development needs none of it. Threat model and open limitations: `docs/security.md`.

## Checklist

1. **Reverse proxy** that overwrites `X-Forwarded-For`; set `TRUSTED_PROXY_HOPS` (production refuses to start without it). See "Proxy contract".
2. **HTTPS**, and `NEXT_PUBLIC_SITE_URL` set to the real origin (canonical URLs, sitemap, og:image).
3. **Secrets:** fresh `AUTH_SECRET` (`openssl rand -base64 48`), `ANTHROPIC_API_KEY`, `SMTP_URL` and `EMAIL_FROM`; schedule the email worker ("Account email delivery"). Keep them in a root-owned environment file.
4. **MongoDB** with authentication, bound to loopback, backed up.
5. **Runner isolation:** systemd cgroup limits, a delegated `RUNNER_CGROUP_ROOT` subtree tested with a memory bomb on the real host, ideally a separate runner uid or service. For a public site consider disposable hosts or VMs (Firecracker, gVisor) so a kernel exploit cannot reach the web server.
6. **Legal:** set `LEGAL_ENTITY_NAME`, `LEGAL_ADDRESS`, `LEGAL_CONTACT_EMAIL`, `LEGAL_JURISDICTION` (the footer shows "operator name to be confirmed" until then). The Terms, Privacy Notice and Cookie Policy are drafts written from what the code does; have a lawyer review them.
7. Use `npm ci` so the lockfile controls versions; re-run `npm audit --omit=dev`.
8. On an existing database after upgrading: `npm run recompute-xp` (site stopped), then `npm run backfill-achievements`.

## Runner limits

The app-level sandbox limits (bubblewrap, seccomp, rlimits, size-capped tmpfs) are in `src/lib/runner.ts`. These need the host or service manager. Do them before exposing the runner to the public.

## Run the web server under systemd with cgroup limits

The code runner starts compilers and learner programs as children of the web server, so a cgroup on the service bounds all of them together.

```ini
[Service]
User=baremetal
Group=baremetal
ExecStart=/usr/bin/npx next start -p 3000
Environment=RUST_TOOLCHAIN_DIR=/usr
MemoryMax=4G
MemorySwapMax=0
TasksMax=512
CPUQuota=400%
# A private /tmp that is size-limited, so work dirs can't fill the real disk or RAM.
PrivateTmp=yes
TemporaryFileSystem=/tmp:size=1G
NoNewPrivileges=yes
ProtectHome=yes
ProtectSystem=strict
ReadWritePaths=/var/lib/baremetal
```

Size the limits to the machine: four concurrent runs (`MAX_CONCURRENT`) each use up to 256 MB of address space for run phases and about 2 GB for compiles.

## Keep secrets and the database away from the runner's uid

Learner code runs as the web server's uid (inside a user namespace with no network). If an attacker ever escapes the sandbox, they get that uid. So:

- Put `.env.local` or the production environment in a root-owned file read by systemd (`EnvironmentFile=`), not in the app's working tree.
- Turn on MongoDB authentication and bind it to localhost or a Unix socket; give the app a user restricted to the `baremetal` database.
- Better: run the runner as a separate service (own uid, no secrets, no database access) that the web app calls over a local socket.

## Also

- A reverse proxy must overwrite `X-Forwarded-For` (`TRUSTED_PROXY_HOPS`), otherwise IP rate limits can be bypassed. See "Proxy contract" below.
- Check the runner after a kernel or bubblewrap upgrade: `npm run verify:content`.

## Proxy contract

Per-IP limits (anonymous runs, tutor IP quota, CAPTCHA, auth endpoints) read the entry of `X-Forwarded-For` that *your* proxy added. For that to be safe:

- The proxy **overwrites** `X-Forwarded-For` with the peer address (nginx: `proxy_set_header X-Forwarded-For $remote_addr;`). `$proxy_add_x_forwarded_for` appends, which keeps the client's own entries.
- The proxy drops any client-supplied `X-Forwarded-Host` (`proxy_set_header X-Forwarded-Host "";`).
- Use `TRUSTED_PROXY_HOPS=1` when nginx overwrites the header with its direct peer address. With a CDN, either validate its signed/client-IP header against trusted CDN ranges and overwrite with the verified client address (still one hop), or explicitly configure every trusted hop to preserve an authenticated chain and set the matching hop count. Do not combine an overwritten one-entry header with a two-hop setting.
- In production the app **refuses to start** if `TRUSTED_PROXY_HOPS` is unset, and logs the value at boot. If a request arrives with a shorter chain than the hop count, it is treated as "unknown": a per-process bucket with a quarter of the normal limit.

## MongoDB authentication

```js
// mongosh, once, as an admin
use baremetal
db.createUser({ user: "baremetal", pwd: passwordPrompt(), roles: [{ role: "readWrite", db: "baremetal" }] })
```

Start `mongod` with `--auth --bind_ip 127.0.0.1` (or a Unix socket), then set
`MONGODB_URI=mongodb://baremetal:<password>@127.0.0.1:27017/baremetal?authSource=baremetal` in the root-owned environment file.

## Maintenance scripts

`npm run recompute-xp` rewrites XP from progress. Run it with the site **stopped**, or a pass recorded during the run can be lost or counted twice.

## Per-job cgroup v2 limits

Set `RUNNER_CGROUP_ROOT` to an empty cgroup-v2 subtree delegated to the runner service uid, with the `memory`, `pids`, `cpu` and `io` controllers enabled in its `cgroup.subtree_control`. The parent containing the web process must remain outside that subtree (cgroup v2's no-internal-process rule). Ask the service manager to delegate these controllers (`Delegate=memory pids cpu io`); create the empty jobs subtree during service setup. Do not make the cgroup hierarchy generally writable.

The runner creates a fresh child per compiler or execution process, sets `memory.max` (2 GiB compile / 512 MiB execution), `memory.swap.max=0`, `pids.max` (512 / 128), `cpu.max=200000 100000`, and `io.weight=default 100`, and joins it before starting Bubblewrap. Failure to create or configure a requested cgroup stops the job; there is no silent fallback. Descendants are killed and the child removed when the sandbox exits. These are per-job limits in addition to the service-wide limits above. Go compilation and QEMU use this path too.

Without `RUNNER_CGROUP_ROOT`, only the existing rlimits, sandbox limits and service-wide cgroup apply. This is the local development default. **Production deployment remains incomplete until the delegated subtree and a memory-bomb check have been exercised on the actual host.** This workspace exposes `/sys/fs/cgroup` read-only, so per-job enforcement could not be tested here.

Compilation now happens entirely on size-capped private tmpfs, with read-only input sources. Only the executable is copied back over a pipe, capped at 64 MiB. Normal compile scratch is 512 MiB and Go scratch is 1 GiB; `/tmp` and `/dev/shm` have separate explicit caps. Runtime work directories, including the QEMU kernel image, are read-only. QEMU serial output goes through bounded scratch; debug results use its stdio backend. Keep the aggregate service memory cap because separate tmpfs mounts can consume their limits together.

## Account email delivery

Signup now requires email verification; new users choose their final password using the verification email. Existing accounts without the verification field remain accepted. Verification tokens expire after 24 hours. Reset links use a fragment so the token is not sent in the initial page request; existing query links remain valid until they expire. SMTP is required for production signup and recovery.

Account emails are queued with encrypted payloads, leased for delivery, and removed on success or after seven days. Requests trigger delivery after their response. Schedule this command every minute under the app uid with the same database and `AUTH_SECRET` settings, so failures and restarts are retried:

```sh
node --conditions=react-server --env-file=.env.local --import tsx scripts/deliver-email.mts
```

Delivery is at least once, so a crash after SMTP accepts a message may cause a duplicate. Monitor delivery failures and expired jobs. Use `npm ci` for deployment so the committed lockfile controls dependency versions; production dependency audit on 29 October 2026 reported zero advisories.

## CSP rendering constraint

The installed Next.js guide (`node_modules/next/dist/docs/01-app/02-guides/content-security-policy.md`) explicitly says nonce CSP is incompatible with Partial Prerendering. This app uses Cache Components and partial shells. Strict nonce enforcement remains a separate rendering migration; simply adding a nonce header would block shell scripts and break hydration. The existing CSP remains enforced, including its `script-src 'unsafe-inline'` limitation. The only application inline script is the static theme bootstrap; it receives no learner data. Both Markdown renderers have executable-markup regression fixtures.
