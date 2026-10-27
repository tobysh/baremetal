# Getting started

## 1. Turn on the AI tutor (pick one)

- **Claude:** get an API key at console.anthropic.com and set `ANTHROPIC_API_KEY=sk-ant-...` in `.env.local`.
- **Free, local:** install [Ollama](https://ollama.com), run `ollama pull qwen2.5-coder:7b`, and set `TUTOR_PROVIDER=ollama` in `.env.local`.

Restart the dev server afterwards.

## 2. Make yourself an admin

```bash
npm run make-admin -- your-email@example.com
```

Then open `/settings/security`, set up 2FA with an authenticator app, and save the backup codes. `/admin` shows analytics, tutor usage and security events.

## 3. Pick a username

Go to `/settings`, choose a username, and you'll appear on `/leaderboard` with a profile at `/u/<username>`. Untick "Public profile" to hide it.

## 4. Running it day to day

Two terminals in `~/code/baremetal`:

```bash
npm run db     # MongoDB
npm run dev    # http://localhost:3000
```

## 5. Save your work

```bash
git add -A && git commit -m "Baremetal"
```

`.env.local` and `.data/` are git-ignored.

## 6. Useful commands

| Command | Use |
|---|---|
| `npm run verify:content` | Check every lesson's solution passes and starter fails |
| `npm run recompute-xp` | Rebuild everyone's XP after changing a lesson's difficulty |
| `npm run test:e2e -- http://localhost:3100` | Security checks against `npx next start -p 3100` |

## 7. Before going online

See "Before going to production" in `README.md`: reverse proxy with HTTPS, real site URL, new `AUTH_SECRET`, SMTP, MongoDB auth, your legal details, and a lawyer's review of the Terms, Privacy Notice and Cookie Policy.
