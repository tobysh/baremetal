# Deployment

This is the single place for everything that must happen before Baremetal is exposed publicly. Local development needs none of it. Threat model and open limitations: `docs/security.md`.

## Checklist

1. **Reverse proxy** that overwrites `X-Forwarded-For`; set `TRUSTED_PROXY_HOPS` (production refuses to start without it). See "Proxy contract".
2. **HTTPS**, and `NEXT_PUBLIC_SITE_URL` set to the real origin (canonical URLs, sitemap, og:image).
3. **Secrets:** fresh `AUTH_SECRET` (`openssl rand -base64 48`), `ANTHROPIC_API_KEY`, `SMTP_URL` and `EMAIL_FROM`; schedule the email worker ("Account email delivery"). Keep them in a root-owned environment file.
4. **MongoDB** with authentication, bound to loopback, backed up.
5. **Runner isolation:** required separate runner service/uid, systemd cgroup limits and a delegated `RUNNER_CGROUP_ROOT` subtree tested with a memory bomb on a disposable deployment host. For a public site consider disposable hosts or VMs (Firecracker, gVisor) so a kernel exploit cannot reach the web server.
6. **Legal:** set `LEGAL_ENTITY_NAME`, `LEGAL_ADDRESS`, `LEGAL_CONTACT_EMAIL`, `LEGAL_JURISDICTION` (the footer shows "operator name to be confirmed" until then). The Terms, Privacy Notice and Cookie Policy are drafts written from what the code does; have a lawyer review them.
7. Use `npm ci` so the lockfile controls versions; re-run `npm audit --omit=dev`.
8. On an existing database after upgrading: `npm run recompute-xp` (site stopped), then `npm run backfill-achievements`.

## Required production environment

In production (`NODE_ENV=production`) `src/instrumentation.ts` refuses to boot, and lists every problem at once, unless all of these are set: `NEXT_PUBLIC_SITE_URL` (must be `https://`; it is inlined at build time, so set it for `npm run build` too), `LEGAL_ENTITY_NAME`, `LEGAL_ADDRESS`, `LEGAL_CONTACT_EMAIL`, `SMTP_URL`, `EMAIL_FROM`, and an `AUTH_SECRET` of at least 32 characters. `TRUSTED_PROXY_HOPS` and `RUNNER_SOCKET` are enforced the same way. The hook does not run during `next build`, so CI builds need no placeholders.

The same-origin check on API routes compares `Origin` only against the host of `NEXT_PUBLIC_SITE_URL` in production and never reads `X-Forwarded-Host`. Development keeps the `Host` comparison so phone and LAN testing works.

## Runner limits

The sandbox implementation is in `src/lib/runner-engine.ts`; public web handlers
use `runner-client.ts`. Production requires a separate runner service. Use the
systemd templates and installation procedure below rather than spawning compiler
children under the web service. The runner keeps four active jobs and up to 24
waiting jobs, with two in-flight requests per account and IP key.

Compiler and execution cgroups have separate limits. The service-wide ceiling
bounds their combined memory and processes. Verify both the per-job limits and
normal compiler performance on the intended machine before exposing the runner.

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

## Database backup and restore

`npm run backup-db` writes `<BACKUP_DIR>/<db>-<UTC timestamp>/` with `manifest.json` (per-collection counts and index definitions) and one `<collection>.jsonl.gz` per collection (canonical Extended JSON, so ObjectId, Date, Long and Double types survive). It is pure Node on the `mongodb` driver, so it works where `mongodump` is not installed (it is not installed on the dev machine). It reads `MONGODB_URI` and `MONGODB_DB` exactly as the app does, never prints credentials, and refuses a `BACKUP_DIR` inside the repository. Files are mode 0600 in a 0700 directory, and a backup directory only appears once it is complete.

| Setting | Default |
| --- | --- |
| `BACKUP_DIR` | `~/baremetal-backups` |
| `BACKUP_KEEP` / `--keep N` | 14 newest backups of this database are kept, older ones are pruned |

```sh
npm run backup-db -- --verify                       # back up, then prove it restores
npm run restore-db -- ~/baremetal-backups/baremetal-20261029T020000Z --verify   # check any backup, touches only a throwaway db
npm run restore-db -- <dir>                         # restore into an EMPTY database
npm run restore-db -- <dir> --force                 # replace the backed-up collections in a non-empty database
npm run restore-db -- <dir> --db baremetal_copy     # restore somewhere else
```

`--verify` restores into a throwaway database named `<db>_verify_<id>`, compares per-collection document counts with the manifest, and drops it. Restore refuses to touch a database that already holds documents unless `--force` is given. Stop the site before a real restore. Consistency note: the dump reads collections one after another without a snapshot, so it is a fuzzy point-in-time copy; fine for this app (progress and the XP ledger are idempotent upserts), but for a stricter copy run it during a quiet period or use `mongodump --oplog` on a replica set. TTL collections (`sessions`, `events`, ...) are restored with their indexes, so expired rows disappear again on their own.

**Scheduling (systemd timer).** `/etc/systemd/system/baremetal-backup.service`:

```ini
[Service]
Type=oneshot
User=baremetal-backup
WorkingDirectory=/srv/baremetal
EnvironmentFile=/etc/baremetal/backup.env   # MONGODB_URI (read-only role), MONGODB_DB, BACKUP_DIR=/var/backups/baremetal
ExecStart=/usr/bin/npx tsx scripts/backup-db.mts --verify
ExecStartPost=/usr/local/bin/baremetal-offsite.sh
UMask=0077
```

`/etc/systemd/system/baremetal-backup.timer`:

```ini
[Timer]
OnCalendar=*-*-* 03:15:00
RandomizedDelaySec=10min
Persistent=true
[Install]
WantedBy=timers.target
```

Enable with `systemctl enable --now baremetal-backup.timer`. A failed backup or failed `--verify` exits non-zero, so the unit shows as failed: alert on that (`OnFailure=` or monitoring of `systemctl is-failed`). Give the backup user a MongoDB role with only `read` (`backup` also works) on the app database, not the app's `readWrite` account.

**Off-host copy.** A backup on the same disk does not survive the disk or the host. Copy the newest directory elsewhere after each run, for example `rclone copy` or `rsync` to another machine or object storage with versioning, and keep at least one copy that the web host cannot delete (append-only or a separate account). Test a real restore from the off-host copy at least once a quarter.

**Encrypt before it leaves the host.** The database holds password hashes, TOTP secrets, session-adjacent data and email addresses. Treat a backup as the most sensitive file on the server. Encrypt the archive before copying, e.g. in `baremetal-offsite.sh`:

```sh
latest=$(ls -d "$BACKUP_DIR"/baremetal-* | sort | tail -1)
tar -C "$BACKUP_DIR" -cf - "$(basename "$latest")" | age -r "$AGE_PUBLIC_KEY" > "$latest.tar.age"
# or: ... | gpg --encrypt --recipient backups@example.org > "$latest.tar.gpg"
```

Keep the private key off the web host (the host only needs the public key). Pruning on the host removes old plain directories; remove old `.tar.age` files on the same schedule. Note `AUTH_SECRET` is not in the database: back it up separately, since encrypted columns and the email queue need it to be read after a restore.

## Separate runner service (required by the production app)

The web app now requires `RUNNER_SOCKET` in production and returns 503 when the
runner is unavailable. It never falls back to execution under the web user.
Development without that setting and curriculum verification scripts can still
use the direct engine. Deploy web and runner from the same commit so both resolve
the same stable exercise IDs and authored tests.

Templates are in `deploy/systemd/`. The assumed layout is:

- `/opt/baremetal-web`: web release, readable by `baremetal-web`.
- `/opt/baremetal-runner`: separate read-only runner release, readable by
  `baremetal-runner`; no `.env*`, database files, backups or application secrets.
- `/etc/baremetal/web.env`: root-owned mode 0600; read by systemd for the web unit
  only. It contains `TRUSTED_PROXY_HOPS=1` and the real application/database/email
  settings. Do not override `RUNNER_SOCKET` there.
- `/run/baremetal-runner/runner.sock`: mode 0660 inside a mode-0750 directory,
  owned by `baremetal-runner:baremetal-exec`. Add only the web user to the shared
  execution group. The web user can connect but cannot replace the socket.

Create separate system users `baremetal-web` and `baremetal-runner` and the
`baremetal-exec` group. Install Node 22+, compiler toolchains under `/usr`,
Bubblewrap and QEMU. Install the web release with `npm ci` and build it; create
its `.next/cache` directory writable by the web user. For the runner release,
copy only `package.json`, `package-lock.json`, `tsconfig.json`, `content/`,
`scripts/runner-service.mts`, `scripts/runner-host.mts`, and these library files:
`runner-engine.ts`, `runner-types.ts`, `runner-protocol.ts`, `runner-resources.ts`,
`gamification.ts`, `learning.ts`, plus the `curriculum/` directory. Run `npm ci`
there as the release installer, not the service user. The runner uses the `tsx`
runtime, so retain development dependencies. Keep both releases root-owned and
unwritable by service users. Do not copy the web working directory wholesale.

Install the two unit files and run `systemctl daemon-reload`, then start the
runner followed by the web unit. The runner host entrypoint moves its supervisor
into a child cgroup and enables the delegated `memory`, `pids`, `cpu` and `io`
controllers in the empty jobs subtree. Every execution joins its own job cgroup
before Bubblewrap starts. Production runner startup requires this configuration.
The template uses the system service path
`/sys/fs/cgroup/system.slice/baremetal-runner.service`; update its
`ReadWritePaths` if the unit is placed in another slice.

The runner has a private network, Unix/netlink-only socket creation, no web environment,
and inaccessible web release, secrets and database directories. Netlink sockets remain available for Bubblewrap to initialise isolated loopback interfaces. Its service
resource ceiling bounds all children together; job cgroups bound each compile or
run. This reduces exposure but does not turn a shared-kernel host into a VM.

### Deployment verification and recovery

These checks need a disposable copy of the real deployment host and are **not
verified by the development workspace**:

1. Confirm both units use different numeric users and that the web environment
   is absent from the runner's process environment.
2. Run `sudo node --import tsx scripts/runner/host-checks.mts --disposable` from a
   checkout containing the validation script. It verifies secret/database
   isolation in the runner's mount/network namespaces, deliberately exercises
   cgroup memory pressure, checks the process limit, and verifies cleanup and a
   subsequent healthy run. Never run pressure probes on a shared production host.
3. Run `npm run test:runner` on a disposable verification host for all five
   languages, admission limits and disconnect cleanup. This suite launches its
   own test-mode runner; step 2 validates the actual systemd runner.
4. Stop the runner: reading lessons and saved progress must still work, and Run
   must report temporary unavailability. Restart it and verify Run/Test recover.
5. Configure SMTP and schedule the email worker, then verify signup and password
   recovery using a real mailbox. Verify backup restore to a separate database.

The daemon refuses to overwrite an existing socket. A clean stop removes it;
`RuntimeDirectory` is removed by systemd after a crash/restart. For manual runs,
remove a stale socket only after confirming its owner has stopped.

Roll out the runner before switching web traffic. If isolation fails, keep
execution unavailable while repairing or rolling back both releases together;
do not restore in-process production execution. Monitor structured runner logs
for `runner_busy`/`runner_failure`, request duration, active/queued jobs and
cancellations. Logs intentionally omit code, secrets and admission identities.
