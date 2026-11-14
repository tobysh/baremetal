// Ordinary sandboxed driver checks; no hostile resource or production-host probes.
import assert from "node:assert/strict";
import { findExercise } from "../../../src/lib/curriculum";
import { toPublicExercise } from "../../../src/lib/curriculum/types";
import { referenceTranscript, testExercise } from "../../../src/lib/runner";
import { compareTranscripts, parseTranscript } from "../../../src/lib/verdict";

const found = findExercise("asm-first-steps/hello-registers");
assert(found?.exercise.driver, "assembly exercise must load its independent driver");
const ex = found.exercise;
assert.equal(ex.harness, undefined, "no duplicate same-process marker harness remains");
const publicExercise = JSON.stringify(toPublicExercise(ex));
assert(!publicExercise.includes('"driver"') && !publicExercise.includes('"solution"'));
const seeds = [Buffer.alloc(16, 0x11), Buffer.alloc(16, 0xa7)];
const transcripts = [];
for (const seed of seeds) {
  const solution = await testExercise(ex, ex.solution, { seed });
  assert(solution.allPassed, JSON.stringify(solution));
  const reference = await referenceTranscript(ex, seed);
  assert(reference.ok, reference.ok ? "" : reference.reason);
  transcripts.push(reference);
}
assert.equal((await testExercise(ex, ex.starter)).allPassed, false, "zero-return starter must fail");
assert(!compareTranscripts(transcripts[1].transcript, parseTranscript(transcripts[0].text), null).intact, "another seed's transcript cannot replay");
const damaged = parseTranscript(transcripts[0].text.replace(/\nE\n$/, "\n"));
assert(!compareTranscripts(transcripts[0].transcript, damaged, null).intact, "incomplete records fail closed");
const badBuild = await testExercise({ ...ex, driver: "invalid C driver source" }, ex.solution);
assert.equal(badBuild.allPassed, false, "a broken grader must not pass");
assert.match(badBuild.error ?? "", /could not be prepared/);
const badRun = await testExercise({ ...ex, driver: 'BM_DRIVER { BM_CASE("unfinished"); bm_sys_(231, 1, 0, 0); }' }, ex.solution);
assert.equal(badRun.allPassed, false, "abnormal reference termination must not pass");
assert.match(badRun.error ?? "", /could not be prepared/);
const missingSymbol = await testExercise(ex, ex.starter.replaceAll("add_two", "wrong_name"));
assert.equal(missingSymbol.compiled, false);
assert.match(missingSymbol.error ?? "", /add_two/);
console.log("Assembly external driver, seeded replay, incomplete records, grader failures, ABI and public-field checks passed.");
