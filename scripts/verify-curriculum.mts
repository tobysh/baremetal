// Checks every lesson: the reference solution must pass all tests, and the
// starter code must compile but not pass. Usage:
//   npm run verify:content            (all tracks)
//   npm run verify:content -- c-memory (one track)
import { getTracks } from "../src/lib/curriculum/index";
import { testExercise } from "../src/lib/runner";

const only = process.argv[2];
let failed = 0;
const ids = new Set<string>();

for (const track of getTracks()) {
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
    if (!start.compiled) problems.push(`starter does not compile (it should, so beginners start from a working state):\n${start.compileOutput}`);
    else if (start.allPassed) problems.push("starter already passes all tests");

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

    const status = problems.length ? "FAIL" : "ok  ";
    console.log(`  ${status} ${lesson.slug} (${sol.results.length} tests${ch ? ", challenge checked" : ""})`);
    for (const p of problems) console.log(`       ${p.replaceAll("\n", "\n       ")}`);
    if (problems.length) failed++;
  }
}
console.log(failed ? `\n${failed} lesson(s) need fixing.` : "\nAll lessons verified.");
process.exit(failed ? 1 : 0);
