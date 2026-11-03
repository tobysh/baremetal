# Documentation

Docs describe how the system works **now**. Decisions that are finished and plans that were carried out live in git history, not here; if a page records a date, it is a measurement or a verification record, not a promise.

## Understand it

| Page | Read it to… |
|---|---|
| [architecture.md](architecture.md) | See the components, the run/test request path, the sandbox, the tutor, auth, data model and web-app conventions |
| [gamification.md](gamification.md) | Understand XP, levels, seals, challenges, reviews, the Codex and weekly rhythm |
| [security.md](security.md) | Know what the sandbox and integrity checks do, and the limitations that are accepted |
| [learning-validation.md](learning-validation.md) | Find the admin cohort definitions, the regression harness, and the manual accessibility and beginner-observation protocols |

## Build on it

| Page | Read it to… |
|---|---|
| [../CONTRIBUTING.md](../CONTRIBUTING.md) | Set up, pick the right checks, follow the repo's rules |
| [content-authoring.md](content-authoring.md) | Add or change lessons, tracks and challenges |
| [configuration.md](configuration.md) | Look up an environment variable |
| [language-interop-feasibility.md](language-interop-feasibility.md) | See why C can't yet be linked into Rust/Go lessons, and what would change that |

## Run it in production

| Page | Read it to… |
|---|---|
| [deployment-hardening.md](deployment-hardening.md) | Work through the pre-launch checklist: proxy, HTTPS, secrets, MongoDB auth, the runner service and cgroups, email worker, backups |
| [capacity.md](capacity.md) | See measured throughput of one runner host |
| [interactive-shell-isolation.md](interactive-shell-isolation.md) | Understand the microVM shell design, trust boundaries and findings |
| [shell-deployment.md](shell-deployment.md) | Install the shell on a dedicated host (not yet exercised on a real one) |
| [`../scripts/shell/hostile/README.md`](../scripts/shell/hostile/README.md) | Run the hostile-workload suite on a disposable VM |

## Track notes (security curriculum)

Scope, models and limits of individual courses: [security-labs.md](security-labs.md), [offensive-infrastructure-lab.md](offensive-infrastructure-lab.md), [toy-maldev.md](toy-maldev.md).

## Keeping docs honest

- Change the doc in the same pull request as the behaviour it describes.
- Prefer pointing at the code (`file.ts`, function name) over copying numbers that drift. When a number matters, say where it is defined.
- Don't add status reports, handoffs or plans here. Put open work in issues, and put lasting decisions in the page they affect.
- Don't leave links to deleted pages: `grep -rn "docs/" --include=*.md .` before removing one.
