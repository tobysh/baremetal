# Baremetal

[![CI](https://github.com/tobysh/baremetal/actions/workflows/ci.yml/badge.svg)](https://github.com/tobysh/baremetal/actions/workflows/ci.yml)

**Learn low-level programming by writing real programs.** Baremetal is an interactive learning platform for C, Rust, Go, x86-64 assembly, and operating-system fundamentals. Work through short lessons in your browser, run your code against hidden tests, and ask an AI tutor for guidance when you get stuck.

The goal is to help you understand what your code is doing. The tutor offers explanations and hints instead of simply giving away the solution.

## Explore the curriculum

- **Start from the basics:** learn to program in C, then build confidence with functions, data, memory, and pointers.
- **Learn systems languages:** study Rust and Go, from core language features to HTTP services, concurrency, and testing.
- **Understand the machine:** write x86-64 assembly and build parts of an operating system that runs in an emulated computer.
- **Practice security with synthetic targets:** investigate C bugs, toy cryptography, packet captures, digital forensics, detection rules, inert sample formats, and fictional application-security scenarios.

Security exercises use made-up data and simulated systems. They do not connect to real targets. See the [acceptable use and privacy information](content/legal/).

## What you get

- An in-browser editor with instant run and test feedback.
- Hidden tests that check a range of inputs, with explanations for common failures.
- Optional hints and an AI tutor powered by Anthropic or a local Ollama model.
- Progress tracking, reviews, challenges, achievements, and public profiles.
- A sandboxed code runner with no network access for learner programs.

## Run Baremetal locally

Baremetal is a Linux-first project. To run the app and its exercises, install:

- Node.js 22 or newer and npm.
- MongoDB.
- GCC (the Rust linker), Rust, Go, and [Bubblewrap](https://github.com/containers/bubblewrap) for the code runner.
- QEMU (`qemu-system-x86_64`) to run the operating-system lessons.

Clone the repository, install dependencies, and create the local MongoDB data directory:

```bash
git clone https://github.com/tobysh/baremetal.git
cd baremetal
npm ci
mkdir -p .data/db
```

Create `.env.local` in the project root. At minimum, set an application secret of 32 or more characters:

```dotenv
MONGODB_URI=mongodb://127.0.0.1:27017
MONGODB_DB=baremetal
AUTH_SECRET=replace-with-output-from-openssl-rand-base64-48
```

Generate a secret with `openssl rand -base64 48`. The local environment file and database directory are ignored by Git. The AI tutor uses Anthropic when `ANTHROPIC_API_KEY` is set. To use Ollama instead, set `TUTOR_PROVIDER=ollama` and configure `OLLAMA_HOST` and `OLLAMA_MODEL` if needed. The app itself works without a tutor provider configured.

Start MongoDB and the app in separate terminals:

```bash
npm run db
```

```bash
npm run dev
```

Open [http://localhost:3000](http://localhost:3000). To enable account email verification and password recovery locally, configure `SMTP_URL` and `EMAIL_FROM` too.

## Useful commands

| Command | Purpose |
| --- | --- |
| `npm run dev` | Start the development server |
| `npm run build` / `npm run start` | Build and serve the production app |
| `npm run lint` | Check code style and common errors |
| `npx tsc --noEmit` | Type-check the TypeScript code |
| `npm run verify:content` | Check lesson starters, solutions, and hidden tests |
| `npm run check:ids` | Ensure published lesson IDs remain stable for learner progress |
| `npm run check:learning` | Check recommendations, prerequisites, cohort calculations and diagnostics |
| `npm run test:runner` | Verify the runner socket, all languages, admission and cancellation |
| `npm run test:regressions` | Run isolated auth, budget, API and browser checks in a temporary production build (requires MongoDB) |
| `npm run check:theme` | Check the design-token rules |
| `npm run test:e2e -- http://localhost:3100` | Run the auth and security checks against a running server |

The full content verifier needs the relevant compiler toolchains, Bubblewrap, and QEMU. See [CI](.github/workflows/ci.yml) for the automated checks and toolchain setup.

## For contributors

Curriculum lessons live under `content/<track>/<lesson>/`. A lesson usually includes `lesson.md`, `task.md`, `hints.md`, `starter.*`, `solution.*`, and either hidden `tests.*` or `cases.json`. Keep lesson IDs stable: they are used to store learner progress. Run `npm run verify:content -- <track>` while developing a track, then run the full verifier before opening a pull request.

For code changes, run lint and type-checking as well. Please keep security exercises synthetic and keep learner code execution inside the sandbox.

## How it works

- **Curriculum:** Markdown and code files define lessons, tasks, hints, starters, solutions, and tests.
- **Runner:** Production execution uses a separate Unix-socket service; GCC, Rust, and Go toolchains run inside Bubblewrap. Kernel exercises boot in QEMU with no disk or network.
- **Tutor:** Anthropic or Ollama provides guided help. Authored hints and common-error explanations remain available without a tutor call.
- **Accounts:** password hashing, secure sessions, optional two-factor authentication, progress export, and account deletion.
- **Privacy:** analytics are consent-based. See the [Privacy Notice](content/legal/privacy.md) and [Cookie Notice](content/legal/cookies.md).

## Documentation

- [Security model and known limitations](docs/security.md)
- [Deployment hardening checklist](docs/deployment-hardening.md)
- [Learning validation and cohort definitions](docs/learning-validation.md)
- [Gamification and review system](docs/gamification.md)
- [Legal notices](content/legal/)

Production startup requires `RUNNER_SOCKET`; use the systemd templates in `deploy/systemd/` and the deployment checklist to install the separate runner service. Production deployment needs additional setup for a trusted reverse proxy, HTTPS, MongoDB authentication, email delivery, runner isolation, and resource limits. Follow the [deployment checklist](docs/deployment-hardening.md) before exposing an instance publicly.

## License

There is currently no `LICENSE` file in this repository. Until a license is added, the source is available to view on GitHub, but reuse and redistribution are not granted. Contact the maintainer about licensing.
