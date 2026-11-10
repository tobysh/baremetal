// Checks every lesson: the reference solution must pass all tests, and the
// starter code must compile but not pass. Usage:
//   npm run verify:content            (all tracks)
//   npm run verify:content -- c-memory (one track)
import { getTracks } from "../src/lib/curriculum/index";
import { assembleParsons, canonicalSubmission, checkPractice, fillBlanks, toPublicPractice } from "../src/lib/curriculum/practice";
import { practiceIdOf } from "../src/lib/gamification";
import { referenceTranscript, testExercise } from "../src/lib/runner";
import { toPublicExercise } from "../src/lib/curriculum/types";

import { validateLearningPaths } from "../src/lib/learning";

validateLearningPaths(getTracks());
const only = process.argv[2];
let failed = 0;
const ids = new Set<string>();

/**
 * The solution with every top-level function except main reduced to returning a zero value
 * (`{}` for void). Relies on the house style: the signature line ends in `{` and the body ends
 * with a `}` alone at the start of a line.
 */
function zeroStub(solution: string): string {
  const out: string[] = [];
  const lines = solution.split("\n");
  for (let i = 0; i < lines.length; i++) {
    const m = /^((?:[A-Za-z_][\w\s*]*?[\s*]))([A-Za-z_]\w*)\s*\([^;]*\)\s*\{\s*$/.exec(lines[i]);
    const end = m && m[2] !== "main" ? lines.indexOf("}", i + 1) : -1;
    if (!m || end < 0) { out.push(lines[i]); continue; }
    const ret = m[1].replace(/\b(static|inline|extern)\b/g, "").trim();
    out.push(lines[i], ret === "void" ? "" : `    static ${ret} bm_zero; return bm_zero;`, "}");
    i = end;
  }
  return out.join("\n");
}

const tracks = getTracks(only);
if (only && tracks.length === 0) throw new Error(`Unknown track: ${only}`);
for (const track of tracks) {
  if (only && track.slug !== only) continue;
  console.log(`\n${track.title} (${track.slug}) — ${track.lessons.length} lessons`);
  for (const lesson of track.lessons) {
    const ex = lesson.exercise;
    const problems: string[] = [];
    if (ids.has(ex.id)) problems.push(`duplicate exercise id ${ex.id}`);
    ids.add(ex.id);
    if (lesson.summary.length > 160) problems.push(`summary is ${lesson.summary.length} chars (max 160 for meta descriptions)`);
    if (ex.hints.length < 2) problems.push("needs at least 2 hints");

    const sol = await testExercise(ex, ex.solution);
    if (!sol.compiled) problems.push(`solution does not compile:\n${sol.compileOutput}`);
    else if (!sol.allPassed) {
      const bad = sol.results.filter((r) => !r.passed).map((r) => `  - ${r.name}: ${r.message}`);
      problems.push(`solution fails:\n${bad.join("\n")}${sol.error ? `\n  error: ${sol.error}` : ""}`);
    }
    if (sol.results.length === 0 && sol.compiled) problems.push("no test results were reported");

    const start = await testExercise(ex, ex.starter);
    // Transfer and design lessons start from an empty file, so the only acceptable failure is the missing main().
    const emptyStart = (lesson.tags.includes("transfer") || lesson.tags.includes("design")) && /undefined reference to `main'/.test(start.compileOutput) && !/\.c:\d+:\d+: error/.test(start.compileOutput);
    if (!start.compiled && emptyStart) { /* expected: the learner writes main */ }
    else if (!start.compiled) problems.push(`starter does not compile (it should, so beginners start from a working state):\n${start.compileOutput}`);
    else if (start.allPassed) problems.push("starter already passes all tests");

    if (ex.driver) {
      // Out-of-process verdict lessons (driver.c, src/lib/verdict.ts): the solution must pass under
      // two fixed, different seeds; the driver must have fixed and seeded checks, and the seeded ones
      // must actually change with the seed; a stub that returns zero from every function must fail;
      // and nothing server-only may appear in the public exercise.
      const seeds = [Buffer.alloc(16, 0x11), Buffer.alloc(16, 0xa7)];
      for (const seed of seeds) {
        const r = await testExercise(ex, ex.solution, { seed });
        if (!r.allPassed) problems.push(`driver: solution fails with seed ${seed.toString("hex")}:\n${r.results.filter((t) => !t.passed).map((t) => `  - ${t.name}: ${t.message}`).join("\n")}${r.error ? `\n  error: ${r.error}` : ""}`);
      }
      const [t1, t2] = await Promise.all(seeds.map((seed) => referenceTranscript(ex, seed)));
      if (!t1.ok || !t2.ok) problems.push(`driver: ${!t1.ok ? t1.reason : !t2.ok ? t2.reason : ""}`);
      else {
        if (!t1.transcript.cases.some((c) => c.fixed)) problems.push("driver: has no fixed edge cases (BM_CASE)");
        if (!t1.transcript.cases.some((c) => !c.fixed)) problems.push("driver: has no seeded cases (BM_SEEDED)");
        if (t1.transcript.cases.map((c) => c.name).join("\0") !== t2.transcript.cases.map((c) => c.name).join("\0")) problems.push("driver: the list of checks depends on the seed (names and order must be fixed)");
        const seeded = (t: typeof t1.transcript) => JSON.stringify(t.cases.filter((c) => !c.fixed));
        if (seeded(t1.transcript) === seeded(t2.transcript)) problems.push("driver: seeded cases do not change with the seed (draw their inputs with bm_int/bm_fill)");
      }
      const stub = zeroStub(ex.solution);
      const s = await testExercise(ex, stub);
      if (!s.compiled) problems.push(`driver: the zero-returning stub did not compile (check zeroStub in this script):\n${s.compileOutput}`);
      else if (s.allPassed) problems.push("driver: a stub that returns zero from every function passes");
      const pub = JSON.stringify(toPublicExercise(ex));
      if (/"(driver|solution|harness|hints)":/.test(pub) || pub.includes(ex.driver.trim().slice(0, 80))) problems.push("driver: the public exercise exposes server-only fields");
    }

    const ch = lesson.challenge?.exercise;
    if (ch) {
      // A challenge must be solvable, must not be satisfied by the plain lesson solution, and must start from a failing, compiling state.
      const csol = await testExercise(ch, ch.solution);
      if (!csol.compiled) problems.push(`challenge solution does not compile:\n${csol.compileOutput}`);
      else if (!csol.allPassed) problems.push(`challenge solution fails:\n${csol.results.filter((r) => !r.passed).map((r) => `  - ${r.name}: ${r.message}`).join("\n")}`);
      const plain = await testExercise(ch, ex.solution);
      if (plain.allPassed) problems.push("the plain lesson solution already passes the challenge, so it is not a challenge");
      const cstart = await testExercise(ch, ch.starter);
      if (!cstart.compiled) problems.push(`challenge starter does not compile:\n${cstart.compileOutput}`);
      else if (cstart.allPassed) problems.push("challenge starter already passes");
    }

    const pr = lesson.practice;
    if (pr) {
      // Parsons and blanks assemble source: the canonical answer must pass the lesson's tests, and a distractor must break it.
      const runs = (pr.kind === "parsons" || pr.kind === "blanks") && pr.runTests !== false;
      const canonical = checkPractice(pr, canonicalSubmission(pr));
      if (!canonical.valid || !canonical.exact) problems.push("practice: the canonical answer is not accepted by the checker");
      else if (runs && canonical.code) {
        const r = await testExercise(ex, canonical.code);
        if (!r.compiled) problems.push(`practice: canonical answer does not compile:\n${r.compileOutput}`);
        else if (!r.allPassed) problems.push(`practice: canonical answer fails the lesson tests:\n${r.results.filter((t) => !t.passed).map((t) => `  - ${t.name}: ${t.message}`).join("\n")}`);
      }
      if (pr.kind === "parsons" && runs) {
        for (const d of pr.distractors) {
          const withD = [pr.lines[0], d, ...pr.lines.slice(1)];
          const r = await testExercise(ex, assembleParsons(pr, withD));
          if (r.compiled && r.allPassed) problems.push(`practice: distractor ${JSON.stringify(d)} still passes the tests when added, so it is not a distractor`);
        }
        const wrong = [...pr.lines].reverse();
        if (wrong.join("\n") !== pr.lines.join("\n")) {
          const r = await testExercise(ex, assembleParsons(pr, wrong));
          if (r.compiled && r.allPassed) problems.push("practice: the reversed order also passes the tests, so the order is not what is being taught");
        }
      }
      if (pr.kind === "blanks" && runs) {
        const swapped = [...pr.answers].reverse();
        if (swapped.join("\0") !== pr.answers.join("\0")) {
          const r = await testExercise(ex, fillBlanks(pr, swapped));
          if (r.compiled && r.allPassed) problems.push("practice: the answers in reverse order also pass the tests");
        }
      }
      // The public view must not carry answers.
      const json = JSON.stringify(toPublicPractice(pr, ex.id, lesson.difficulty));
      if (/"(answers?|accepted|pairs|bugLine|explain|distractors|runTests|base)":/.test(json)) problems.push("practice: the public view exposes an answer field");
      if (pr.kind === "bug" && lesson.practice && json.includes(pr.explain ?? "\u0000")) problems.push("practice: the public view exposes the explanation");
      if (practiceIdOf(ex.id) !== toPublicPractice(pr, ex.id, lesson.difficulty).id) problems.push("practice: id mismatch");
    }

    for (const [qi, q] of (lesson.quiz ?? []).entries()) {
      const ok = checkPractice(q, canonicalSubmission(q));
      if (!ok.valid || !ok.exact) problems.push(`quiz[${qi}]: the canonical answer is not accepted by the checker`);
      if (/"(answers?|accepted|pairs|bugLine|explain|distractors)":/.test(JSON.stringify(toPublicPractice(q, ex.id, lesson.difficulty, `${ex.id}#q${qi}`)))) problems.push(`quiz[${qi}]: the public view exposes an answer field`);
    }

    const status = problems.length ? "FAIL" : "ok  ";
    console.log(`  ${status} ${lesson.slug} (${sol.results.length} tests${ch ? ", challenge checked" : ""}${pr ? `, practice (${pr.kind}) checked` : ""})`);
    for (const p of problems) console.log(`       ${p.replaceAll("\n", "\n       ")}`);
    if (problems.length) failed++;
  }
}
console.log(failed ? `\n${failed} lesson(s) need fixing.` : "\nAll lessons verified.");
process.exit(failed ? 1 : 0);
