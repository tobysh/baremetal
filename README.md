# Baremetal

Learn low-level programming from absolute zero. Learners read short lessons, write C, Rust, Go and x86-64 assembly in an in-browser editor, get instant results from hidden tests run in a sandbox, and can ask an AI tutor that guides them without handing over answers.

**Courses (97 lessons):** Programming from Zero (C) → Pointers & Memory in C → Systems Programming in C → C in Practice → Rust Foundations → Rust Traits & Lifetimes → Rust in Practice → Build an HTTP Server in Rust → Go Foundations → Assembly: First Steps → Build an Operating System (in progress).

## Running it locally

Prerequisites (already installed on this machine): Node 24 via nvm, MongoDB 8 in `~/.local/opt` (linked into `~/.local/bin`), Rust via rustup, Go (`/usr/bin/go`, used by the sandbox), `gcc`, `bwrap` (bubblewrap) for the code sandbox, and `qemu-system-x86_64` (`pacman -S qemu-system-x86`) for the OS-development track.

```bash
npm install
npm run db            # terminal 1: MongoDB, data in .data/db
npm run dev           # terminal 2: http://localhost:3000
```

Settings live in `.env.local` (never commit it). For the AI tutor, either set `ANTHROPIC_API_KEY`, or install [Ollama](https://ollama.com), run `ollama pull qwen2.5-coder:7b`, and set `TUTOR_PROVIDER=ollama`.

Make yourself an admin (then turn on 2FA at `/settings/security` to open `/admin`):

```bash
npm run make-admin -- you@example.com
```

## Scripts

| Script | What it does |
|---|---|
| `npm run dev` / `build` / `start` | Next.js dev server, production build, production server |
| `npm run db` | Start the local MongoDB |
| `npm run verify:content [-- track]` | Checks every lesson: the reference solution passes, the starter compiles but fails |
| `npm run test:e2e -- <url>` | 46 end-to-end security checks against a running server (sign-up, 2FA, lockout, CAPTCHA, reset, authorization) |
| `npm run make-admin -- <email>` | Promote an account to admin |

## How it fits together

- **Content** is plain files in `content/<track>/<NN-lesson>/`: `lesson.md`, `task.md`, `hints.md`, `starter.*`, `solution.*` and `tests.*` (or `cases.json` for output-checked lessons). Legal text is in `content/legal/`.
  - **Assembly tracks** (`"language": "asm"` in `track.json`): learners edit `starter.s` (GNU as, `.intel_syntax noprefix`, System V AMD64 calling convention: args in `rdi rsi rdx rcx r8 r9`, result in `rax`). The hidden `tests.c` uses the same `CHECK` harness as C lessons, declares the learner's functions `extern`, and has its own `main`; the runner links `gcc -no-pie starter.s tests.c`. Every `.s` ends with `.section .note.GNU-stack,"",@progbits`. No UBSan applies; functions are leaf routines (no libc calls needed). See `content/asm-first-steps/01-hello-registers`.
- **Kernel tracks** (`"language": "kernel"`, the `osdev` course): learners edit `kernel.c`, freestanding C with no libc. The runner links it with a hidden boot stub and runtime from `src/lib/kernel/` and boots it in QEMU (TCG, 64 MB, no network, inside bubblewrap). Hidden `tests.c` defines `kmain()` and runs inside the virtual machine, reporting through the QEMU debug console. See `content/osdev/01-serial-port` for the smallest example and `content/osdev/11-context-switch` for one with assembly.
- **Code runner** (`src/lib/runner.ts`) compiles and runs code with gcc/rustc inside bubblewrap: no network, read-only system, private temp dir, CPU/memory/process/output limits, 5 s timeout. C is built with UBSan to explain undefined behaviour.
- **AI tutor** (`src/lib/tutor.ts`, `/api/tutor`): Claude Haiku 4.5 or a local Ollama model. Costs are kept down by a response cache for repeated first questions, prompt caching, short history and output caps, free pre-written failure explanations (`src/lib/diagnostics.ts`) and authored hints, and per-user/IP/global quotas.
- **Auth** (`src/lib/auth.ts`, `src/app/(auth)/actions.ts`): scrypt passwords, hashed session tokens in `httpOnly` `SameSite=Strict` cookies, TOTP 2FA with backup codes (required for admins), lockout with exponential backoff, proof-of-work CAPTCHA, HIBP and common-password checks.
- **Privacy:** consent-gated first-party analytics (no cookies, no raw IPs), data export and account deletion in `/settings`, TTL indexes that delete data on the schedule stated in the Privacy Notice.

## Before going to production

1. **Run behind a reverse proxy** that sets `X-Forwarded-For` (nginx: `proxy_set_header X-Forwarded-For $remote_addr;`), and set `TRUSTED_PROXY_HOPS`. Without one, clients can fake their IP and get around the per-IP limits (per-account limits still hold).
2. **HTTPS**, and set `NEXT_PUBLIC_SITE_URL` to the real origin (canonical URLs, sitemap, og:image).
3. **Secrets:** a fresh `AUTH_SECRET` (`openssl rand -base64 48`), `ANTHROPIC_API_KEY`, `SMTP_URL` and `EMAIL_FROM` for password-reset emails.
4. **MongoDB** with authentication enabled and not exposed publicly; back it up.
5. **Legal:** fill in `LEGAL_ENTITY_NAME`, `LEGAL_ADDRESS`, `LEGAL_CONTACT_EMAIL`, `LEGAL_JURISDICTION`. The Terms, Privacy Notice and Cookie Policy are drafts written from what the code actually does. Have them reviewed by a qualified lawyer before launch.
6. **Code sandbox:** bubblewrap is solid for a learning site, but for a public deployment consider running the runner on separate, disposable hosts or VMs (e.g. Firecracker or gVisor) so a kernel exploit can't reach the web server.
