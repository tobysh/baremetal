# Writing curriculum content

Everything a learner reads, writes and is tested against lives under `content/`. This guide covers the file layout, what each file does, the rules that keep the tests honest, and how to verify your work. For how the content is loaded and executed, see [architecture.md](architecture.md).

## Layout

```
content/
  <track>/                  e.g. c-memory, go-concurrency, osdev
    track.json              course metadata
    <NN>-<slug>/            lesson folders, ordered by the NN prefix
      lesson.md             frontmatter + the lesson body (markdown)
      task.md               what the learner must write
      hints.md              progressive hints, separated by lines containing only ===
      starter.<ext>         what the editor starts with
      solution.<ext>        reference solution (server-only)
      tests.<ext>           hidden harness  — or —  cases.json for stdout lessons
      challenge.md          optional harder variant (see below)
      challenge-starter.<ext>, challenge-solution.<ext>, challenge-tests.<ext> | challenge-cases.json
  legal/                    Terms, Privacy and Cookie notices (rendered at /terms, /privacy, /cookies)
  .lesson-ids.json          snapshot of published lesson ids (see "Stable ids")
```

Extensions follow the track's `language`: `c` → `.c`, `rust` → `.rs`, `go` → `.go`, `asm` → `.s` for starter and solution with a C `tests.c`, `kernel` → `.c` (`kernel.c`) with a C `tests.c`.

## `track.json`

The `//` comments below are explanatory only; real JSON can't contain them.

```jsonc
{
  "title": "Pointers & Memory in C",
  "summary": "One or two sentences, used as the meta description.",
  "description": "A longer paragraph shown on the course page.",
  "level": "Beginner",          // Beginner | Intermediate | Advanced
  "language": "c",              // c | rust | go | asm | kernel
  "path": "c",                  // start | c | rust | go | low | security  (src/lib/curriculum/paths.ts)
  "order": 1,                   // position within the path
  "prerequisite": "c-data",     // optional track slug; recommendations and the course map use it
  "outcomes": ["Concrete things a learner can do after finishing."],
  "draft": true                 // optional; hides the whole course (see "Drafts")
}
```

The track slug is the folder name. `npm run verify:content` validates prerequisites (missing slugs and cycles fail).

## `lesson.md`

```markdown
---
title: Pointers and Swap
summary: One sentence, at most 160 characters; also the meta description.
minutes: 20
difficulty: medium              # easy | medium | hard | impossible
tags: pointers                  # optional; comma-separated lowercase kebab-case
kind: review                    # optional; omit for a normal lesson
id: c-memory/pointers-swap
mode: harness                   # harness (default) | stdout
---
Lesson body in markdown…
```

- `difficulty` sets the XP (5/10/20/40) and the badge colour. **Changing it later requires `npm run recompute-xp`** with the site stopped.
- `id` is `<track>/<slug>` by convention, where the slug is the folder name without its `NN-` prefix. It is the progress key, so treat it as permanent.
- A **review** lesson consolidates earlier material: no new concepts, familiar ideas combined with small twists, usually `easy` or `medium`, titled `Review: …`.
- Teach before you test: the body explains the idea with small examples before the task asks for anything. Use second person, short paragraphs, and runnable snippets.

## `task.md` and `hints.md`

`task.md` states exactly what to write: function signatures, input and output formats, edge cases the tests will exercise. Everything the hidden tests demand must be derivable from the task.

`hints.md` needs **at least two hints**, ordered from a nudge to nearly the answer, separated by a line containing only `===`. They are served one at a time through `/api/hint`. Hints should not paste the full solution.

## Two kinds of exercise

### `mode: stdout`

The learner's program is run and its output compared with `cases.json`:

```json
[
  { "name": "3 by 4 grid", "expected": "   1   2   3   4\n…" },
  { "name": "1 by 1 grid", "set": { "rows": "1", "cols": "1" }, "expected": "   1\n" },
  { "name": "reads input", "stdin": "5\n", "expected": "25\n" }
]
```

**No hardcodable answers.** If the starter declares its inputs (`int rows = 3;`), a learner could print the expected text and pass. Defend against it:

- Add cases with `"set": { "name": "value" }`. The runner rewrites the first declaration of each named variable in the learner's source (`=`, `:=`, type annotations, comma declarations and quoted values are handled) and recompiles before running. If a variable is missing the case fails with a "keep the starter's variables" message.
- Use at least two variants and include edge values (0, an exact multiple, negatives).
- Tell the learner in `task.md` that other values will be tried.
- Lessons that read `stdin` vary `stdin` per case instead.
- Stdout lessons with nothing to vary (fixed text, hello-world lessons, FizzBuzz) cannot be protected this way; prefer a harness lesson when you can.

`npm run check:set-values` and `npm run check:stdout-order` guard the mechanism.

### `mode: harness`

A hidden test file calls the learner's code with many inputs:

| Language | Harness file | Shape |
|---|---|---|
| C | `tests.c` | Defines `main()`; call the learner's functions and report with `CHECK(name, condition, fmt, ...)`. The `CHECK` macro and prelude are injected by the runner; the harness is appended to the learner's file |
| Rust | `tests.rs` | `#[test]` functions appended to the learner's `main.rs` and run with `--test` |
| Go | `tests.go` | A `package main` file of `TestXxx` functions with its own imports; built as `bm_test.go` and run with `-test.v` |
| Assembly | `tests.c` | A C file declaring the learner's functions `extern`, with its own `main`; linked against the assembled `starter.s` |
| Kernel | `tests.c` | Appended to the learner's `kernel.c`; defines `kmain()` and inspects real hardware state in QEMU. Keep timing assertions loose |

Write harnesses so that:

- **Test names are stable and deterministic.** The server requires the learner's run to produce exactly the ordered list of names the reference solution produces. Do not build names from random values or timing.
- **Several different inputs** are exercised, including edge cases. A function that returns a constant must fail.
- **Failures explain themselves**: say what was expected and what was received (`"got %d"`). This is what the learner reads.
- **Expected values are independent.** Prefer fixed vectors or a separate reference over comparing two things the learner wrote.
- The starter must **compile but fail**; the solution must pass every test.

## Challenges

An optional harder variant, unlocked once the lesson is passed and worth half XP. Add `challenge.md` (the brief) plus `challenge-starter.*`, `challenge-solution.*` and `challenge-tests.*` (or `challenge-cases.json`). The challenge's exercise id is `<lesson id>#challenge`. It must require more reasoning than the lesson: the verifier checks that the challenge solution passes **and that the plain lesson solution does not**.

## Practice (tap and drag)

A lesson may add `practice.json` beside `solution.*`: a short exercise for phones that needs no typing. It pays a quarter of the lesson's XP (minimum 1) once, and never replaces the coding task. Its progress id is `<lesson id>#practice`. The loader validates the file and fails the build on a malformed one.

Common fields: `kind` (`parsons`, `blanks`, `predict` or `bug`), `prompt` (short markdown, at most 600 characters) and optional `explain` (shown only after a correct answer). The browser never receives answers, the canonical order or blank values.

```jsonc
// parsons: put the lines in order. Lines keep their own indentation.
{ "kind": "parsons", "prompt": "...", "lines": ["...", "..."], "distractors": ["..."],
  "base": "int main(void) {\n{{code}}\n}",   // optional: source around the lines, {{code}} once; default is the lines alone
  "runTests": true }                          // optional, default true; false means only the canonical order counts

// blanks: fill {{0}}..{{n-1}} (each exactly once) from a tray; the template is the whole file
{ "kind": "blanks", "prompt": "...", "template": "int *p = {{0}}x;", "tokens": ["&", "*", "x"], "answers": ["&"] }

// predict: one question ...
{ "kind": "predict", "prompt": "...", "code": "...", "choices": ["1", "2"], "answer": 1 }
// ... or a trace of steps, each its own question
{ "kind": "predict", "prompt": "...", "trace": [{ "label": "after line 3, x is", "choices": ["1", "2"], "answer": 0 }] }

// bug: tap the faulty line (1-based); `explain` is required
{ "kind": "bug", "prompt": "...", "code": "...", "bugLine": 7, "explain": "..." }
{ "kind": "choice", "prompt": "...", "code": "(optional)", "choices": ["...", "..."], "answer": 1 }            // answer: [0, 2] makes it select-all
{ "kind": "typed", "prompt": "...", "code": "(optional)", "accepted": ["14"], "caseSensitive": false }       // short typed answer; spaces are collapsed
{ "kind": "match", "prompt": "...", "pairs": [{ "left": "%d", "right": "an int" }, { "left": "%c", "right": "a character" }] }
```

A trace label may start with the register or stack slot it fills in square brackets, for example ``"[rax] after `add rax, rsi`"``. The learner sees a register panel (a name starting with `stack` or `mem` goes in the stack column, anything else is a register), fills each cell by tapping a value or dragging it onto the highlighted cell, and the latest answer for a name is what the cell shows. A backticked snippet in the label highlights the first `code` line containing it. Labels without a prefix get a "step N" cell. Keep `code` to the few instructions being traced.

Rules: a distractor must not equal a correct line; `answers` must be drawn from `tokens` (one token per slot); `answer` and `bugLine` must be in range. For parsons and blanks the assembled source is also what the lesson's hidden tests run against: `verify:content` requires the canonical answer to pass them, every distractor (inserted after the first line) to fail, and a reversed order to fail. A learner's non-canonical arrangement is accepted if it passes the same tests. Stdout lessons that use `set` variables need the variables to survive in the assembled source. Set `"runTests": false` for a fragment that cannot run on its own.

In the lesson workspace a lesson with `practice.json` gets a "Practice (tap & drag)" tab beside "Code"; the editor stays mounted behind it. `PracticePanel` (`src/components/practice/practice-panel.tsx`) shows the prompt, the practice XP against the full task's XP, the Check button (it posts to `/api/practice`), feedback and the reward. Each kind is a component registered in `src/components/practice/registry.ts` (`practiceKinds`), receiving `KindProps` from `types.ts` and reporting its current submission through `onAnswer`. Parsons: tap a line to add it, drag the grip or press and hold a line to reorder, or use the Move up / Move down / Remove buttons (Alt+Arrow also moves the focused line). Blanks: tap a blank then a piece, tap pieces to fill the next blank, tap a filled blank to clear it, or drag a piece onto a blank. Authors: keep each line short (the board scrolls sideways on phones) and give blanks at most a handful of slots.

### Review questions (`quiz.json`)

Review cards (`/review`) test recall with a real question, not a self-grade. A lesson's deck is its `practice.json` plus an optional `quiz.json`: an array of 1-12 questions in the same shapes as `practice.json`, any kind. `quiz.json` questions pay no XP and appear only on review cards; one is picked each time the lesson comes due. Write questions that need recall of the idea (a value, a keyword, the faulty line, a prediction), not ones that restate the lesson heading, and give each an `explain`. `verify:content` checks that the canonical answer is accepted and that the public view hides answers. Lessons without a deck fall back to a card that opens the coding review.

Lesson types for the `?type=` filter are derived, not authored: `lesson` (or `review` for `kind: review`), plus `challenge` when `challenge.md` exists, plus the practice `kind` (`lessonTypes()` in `src/lib/curriculum/lesson-types.ts`).

## Drafts

Work in progress can live in the repo without being published:

- `"draft": true` in `track.json` hides the whole course.
- `draft: true` in a lesson's frontmatter hides one lesson in an otherwise published course.

Drafts are excluded from the catalogue, search, recommendations, static routes and exercise lookup, and from `npm run verify:content`. To verify a draft, run the verifier on a working copy with the flag cleared, then remove the flag in the real tree once it passes. Anything *not* marked draft is published, and a missing file in published content is a hard error (so a broken lesson can't silently vanish).

## Stable ids

Learner progress is keyed by lesson id. Therefore:

- **Never change** a published lesson's folder slug or `id:`.
- Renumbering the `NN-` prefixes is safe and is how you insert a lesson mid-track.
- `npm run check:ids` compares ids against `content/.lesson-ids.json` (and `origin/main` when available) and fails if one has disappeared. New ids are allowed; add them to the snapshot with `npm run check:ids -- --update` and confirm the diff only adds lines.

## Security content

Security lessons use **synthetic data and simulated systems only**: fictional targets, fictional advisory ids, inert bytecode formats, in-memory state machines. They never connect to real hosts, and a lesson must not need broader sandbox privileges. State each lesson's model boundaries honestly in its `task.md` or the track `description` ("a model of control-flow overwrite, not a native exploit"). Per-track notes: [security-labs.md](security-labs.md), [offensive-infrastructure-lab.md](offensive-infrastructure-lab.md), [toy-maldev.md](toy-maldev.md).

## Legal pages

`content/legal/*.md` describe what the code actually stores. When a change adds stored data, a cookie, or a `localStorage` key, update the relevant notice **and** bump `SITE.legal.lastUpdated` in `src/lib/site.ts`. Bump it whenever anything under `content/legal/` changes.

## Verifying your work

```bash
npm run verify:content -- <track>   # every solution passes; every starter compiles but fails; summaries ≤ 160 chars; ≥ 2 hints; ids unique
npm run verify:content              # the whole curriculum (needs gcc, rustc, go, bubblewrap, qemu-system-x86_64)
npm run check:ids                   # no published id disappeared
npm run check:learning              # prerequisites and recommendations still resolve
npm run check:curriculum            # draft isolation and strict loading
```

If `qemu-system-x86_64` is missing, `verify:content -- osdev` reports a bare "triple fault" for every lesson. Run it before a pull request, and run `npm run recompute-xp` (site stopped) if you changed a difficulty or removed a `practice.json` on a database with real learners.

## Design guidance

- Prefer harness lessons and deterministic simulations (fake clocks, allocators, devices, networks, schedulers) over timing assertions. A timing check alone never establishes a general performance claim.
- Give each lesson a concrete learning outcome and an observable completion condition. Explanations, code review and "teach-back" tasks cannot be graded by output alone; do not pretend they can.
- Vary hidden inputs and failure schedules instead of testing a single path.
- Include a public example the learner can actually run (a `main` that exercises the function) so they can explore before testing.
- Check that new exercises don't need toolchain features the runner lacks. For example, C cannot be linked into Rust or Go lessons today ([language-interop-feasibility.md](language-interop-feasibility.md)).
- Record the limits of any simulation: a model demonstrates its stated model, not a real system.
