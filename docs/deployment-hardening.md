# Deployment hardening

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

- A reverse proxy must overwrite `X-Forwarded-For` (`TRUSTED_PROXY_HOPS`), otherwise IP rate limits can be bypassed.
- Check the runner after a kernel or bubblewrap upgrade: `npm run verify:content`.
