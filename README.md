# Baremetal

Learn low-level programming from absolute zero. Learners read short lessons, write C, Rust, Go and x86-64 assembly in an in-browser editor, get instant results from hidden tests run in a sandbox, and can ask an AI tutor that guides them without handing over answers.

Courses and lessons (`npm run verify:content` lists them) across six learning paths: Start here, C, Rust, Go, Machine level and Security. Advanced projects include a shell, interpreter, database and OS continuation; security courses cover fixing C bugs, toy cryptography, packet analysis and digital forensics. Captures and forensic evidence are synthetic; fixture generators live in `scripts/content-gen/`.

## Running it locally

Prerequisites (already installed on this machine): Node 24 via nvm, MongoDB 8, Rust via rustup or a system installation at `/usr` (detected automatically), Go (`/usr/bin/go`, used by the sandbox), `gcc`, `bwrap` (bubblewrap) for the code sandbox, and `qemu-system-x86_64` (`pacman -S qemu-system-x86`) for the OS-development track.

```bash
npm install
npm run db            # terminal 1: MongoDB, data in .data/db
npm run dev           # terminal 2: http://localhost:3000
```

On Arch, either enable MongoDB at boot (`sudo systemctl enable --now mongodb.service`) or start it per session; `npm run db` needs `.data/db` to exist. `.env.local` and `.data/` are git-ignored.

Settings live in `.env.local` (never commit it). For the AI tutor, either set `ANTHROPIC_API_KEY`, or install [Ollama](https://ollama.com), run `ollama pull qwen2.5-coder:7b`, and set `TUTOR_PROVIDER=ollama`.

Make yourself an admin (then turn on 2FA with an authenticator app at `/settings/security`, save the backup codes, and `/admin` opens with analytics, tutor usage and security events):

```bash
npm run make-admin -- you@example.com
```

Pick a username in `/settings` to appear on `/leaderboard` and at `/u/<username>` (untick "Public profile" to hide it).

## Scripts

| Script | What it does |
|---|---|
| `npm run dev` / `build` / `start` | Next.js dev server, production build, production server |
| `npm run db` | Start the local MongoDB |
| `npm run verify:content [-- track]` | Checks every lesson: the reference solution passes, the starter compiles but fails |
| `npm run test:e2e -- <url>` | Expanded end-to-end security checks against a running server (sign-up, 2FA, lockout, CAPTCHA, reset, authorization) |
| `npm run make-admin -- <email>` | Promote an account to admin |
| `npm run recompute-xp` | Rebuild XP after changing a difficulty or level table (site stopped) |
| `npm run backfill-achievements` | Award earned Codex entries |

## How it fits together

- **Content** is plain files in `content/<track>/<NN-lesson>/`: `lesson.md`, `task.md`, `hints.md`, `starter.*`, `solution.*` and `tests.*` (or `cases.json` for output-checked lessons). Legal text is in `content/legal/`.
  - **Assembly tracks** (`"language": "asm"` in `track.json`): learners edit `starter.s` (GNU as, `.intel_syntax noprefix`, System V AMD64 calling convention: args in `rdi rsi rdx rcx r8 r9`, result in `rax`). The hidden `tests.c` uses the same `CHECK` harness as C lessons, declares the learner's functions `extern`, and has its own `main`; the runner links `gcc -no-pie starter.s tests.c`. Every `.s` ends with `.section .note.GNU-stack,"",@progbits`. No UBSan applies; functions are leaf routines (no libc calls needed). See `content/asm-first-steps/01-hello-registers`.
- **Kernel tracks** (`"language": "kernel"`, the `osdev` course): learners edit `kernel.c`, freestanding C with no libc. The runner links it with a hidden boot stub and runtime from `src/lib/kernel/` and boots it in QEMU (TCG, 64 MB, no network, inside bubblewrap). Hidden `tests.c` defines `kmain()` and runs inside the virtual machine, reporting through the QEMU debug console. See `content/osdev/01-serial-port` for the smallest example and `content/osdev/11-context-switch` for one with assembly.
- **Code runner** (`src/lib/runner.ts`) compiles and runs code with gcc/rustc inside bubblewrap: no network, read-only system, private temp dir, CPU/memory/process/output limits, 5 s timeout. C is built with UBSan to explain undefined behaviour.
- **AI tutor** (`src/lib/tutor.ts`, `/api/tutor`): Claude Haiku 5.5 or a local Ollama model. Costs are kept down by a response cache for repeated first questions, prompt caching, short history and output caps, free pre-written failure explanations (`src/lib/diagnostics.ts`) and authored hints, and per-user/IP/global quotas.
- **Auth** (`src/lib/auth.ts`, `src/app/(auth)/actions.ts`): scrypt passwords, hashed session tokens in `httpOnly` `SameSite=Strict` cookies, TOTP 2FA with backup codes (required for admins), lockout with exponential backoff, proof-of-work CAPTCHA, HIBP and common-password checks.
- **Privacy:** consent-gated first-party analytics (no cookies, no raw IPs), data export and account deletion in `/settings`, TTL indexes that delete data on the schedule stated in the Privacy Notice.

## Before going to production

Everything needed (reverse proxy and `TRUSTED_PROXY_HOPS`, HTTPS, secrets, SMTP and the email worker, MongoDB auth, runner isolation and cgroups, legal details and review) is one checklist in [docs/deployment-hardening.md](docs/deployment-hardening.md). The threat model and known limitations are in [docs/security.md](docs/security.md).

## More docs

- `CLAUDE.md`: architecture, commands, theme and content rules. `HANDOFF.md`: current status and next steps.
- `docs/gamification.md`: XP, levels, seals, challenges, Codex, rhythm, reviews.
- `docs/archive/`: finished plans and superseded designs.
