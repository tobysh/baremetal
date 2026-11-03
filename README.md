# Baremetal

[![CI](https://github.com/tobysh/baremetal/actions/workflows/ci.yml/badge.svg)](https://github.com/tobysh/baremetal/actions/workflows/ci.yml)

**Learn low-level programming by writing real programs.** Baremetal teaches C, Rust, Go, x86-64 assembly and operating-system fundamentals. You read a short lesson, write code in the browser, and run it against hidden tests. An AI tutor explains and hints instead of handing over the answer.

## What's in it

- **A curriculum of ~625 lessons in ~56 courses**, as markdown and code files under [`content/`](content/): programming from zero, C (memory, data structures, concurrency, networking, allocators, debugging), Rust, Go (services, concurrency, databases, distributed systems), assembly, and a bare-metal x86-64 kernel that boots in QEMU. A security path covers C bugs, cryptography, packets, forensics, detection and fictional offensive labs; all security exercises use synthetic data and never touch a real target.
- **An in-browser editor** with Run and Test, explained failures, progressive hints and a ⌘K command palette.
- **A sandboxed runner**: compilers and learner programs run under bubblewrap with no network, strict limits, and a verdict protocol designed to resist hardcoded or forged results.
- **A tutor** (Anthropic or a local Ollama model) that never sees the reference solution.
- **Progress and motivation without pressure**: XP, levels, seals, optional challenges, spaced reviews, a weekly rhythm, achievements and opt-in public profiles.
- **Accounts**: scrypt, optional two-factor, email verification, data export and deletion.
- An optional **interactive shell** in throwaway Firecracker microVMs (built, not yet verified on a real host).

Stack: Next.js 16 (App Router, Cache Components, Turbopack), React 19, Tailwind v4, MongoDB, CodeMirror.

## Run it locally

Linux is the supported platform. You need Node.js 22+, MongoDB, and, to run exercises, gcc, Rust, Go, [bubblewrap](https://github.com/containers/bubblewrap) and `qemu-system-x86_64`.

```bash
git clone https://github.com/tobysh/baremetal.git && cd baremetal
npm ci
mkdir -p .data/db
```

Create `.env.local`:

```dotenv
MONGODB_URI=mongodb://127.0.0.1:27017
MONGODB_DB=baremetal
AUTH_SECRET=<output of: openssl rand -base64 48>
# optional: ANTHROPIC_API_KEY=…   (or TUTOR_PROVIDER=ollama)
# optional: SMTP_URL=… and EMAIL_FROM=…  (signup email verification and password recovery)
```

Then, in two terminals:

```bash
npm run db     # MongoDB, data in .data/db
npm run dev    # http://localhost:3000
```

The site works without a tutor provider. All variables are listed in [docs/configuration.md](docs/configuration.md).

## Common commands

| Command | Purpose |
|---|---|
| `npm run dev` / `build` / `start` | Develop, build and serve |
| `npm run lint`, `npx tsc --noEmit` | Style and types |
| `npm run verify:content [-- <track>]` | Every solution passes, every starter compiles but fails |
| `npm run check:ids`, `check:theme`, `check:learning`, `check:curriculum` | Fast guards (stable lesson ids, design tokens, prerequisites, drafts) |
| `npm run test:runner`, `test:regressions`, `test:browser` | Runner transport, isolated full-stack and Playwright suites |
| `npm run test:e2e -- http://localhost:3100` | Auth and security checks against a running server |
| `npm run make-admin -- <email>` | Grant admin |
| `npm run recompute-xp`, `backfill-achievements` | Maintenance (site stopped!) |
| `npm run backup-db`, `restore-db` | Database backup and restore |

[CONTRIBUTING.md](CONTRIBUTING.md) says which checks to run for which change.

## Documentation

Start with the [documentation index](docs/README.md). The most useful pages:

- [Architecture](docs/architecture.md): components, request flows, runner internals, data model.
- [Writing content](docs/content-authoring.md): the lesson format and the rules that keep tests honest.
- [Contributing](CONTRIBUTING.md): setup, checks, conventions.
- [Security model](docs/security.md) and [deployment guide](docs/deployment-hardening.md): read both before exposing an instance publicly. Production needs a reverse proxy, HTTPS, MongoDB authentication, email delivery and a separate runner service.
- [Configuration reference](docs/configuration.md).

Legal notices (Terms, Privacy, Cookies) are in [`content/legal/`](content/legal/).

## License

There is currently no `LICENSE` file. Until one is added, the source is viewable on GitHub but reuse and redistribution are not granted. Contact the maintainer about licensing.
