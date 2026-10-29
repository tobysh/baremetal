// A cold successful submission must not poison the shared reference cache when
// its browser disconnects. Run: node --import tsx scripts/security/runner-cancellation.mts
import assert from "node:assert/strict";
import { randomUUID } from "node:crypto";
import { readdir, readFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import path from "node:path";
import { setTimeout as delay } from "node:timers/promises";
import type { Exercise } from "../../src/lib/curriculum/types";
import { testExercise, withRunnerRequest } from "../../src/lib/runner";

const marker = `cancel-reference-${randomUUID()}`;
const learner = "int answer(void) { return 42; }\nint main(void) { return 0; }";
const exercise: Exercise = {
  id: `security/${marker}`, language: "c", mode: "harness", task: "Regression fixture", hints: [],
  starter: "int answer(void) { return 0; }", solution: `#include <unistd.h>\n/* ${marker} */\nint answer(void) { sleep(1); return 42; }\nint main(void) { return 0; }`,
  harness: 'int main(void) { CHECK("answer", answer() == 42, "wrong answer"); return 0; }',
};
const abort = new AbortController();
let finished = false;
const first = withRunnerRequest([marker], abort.signal, () => testExercise(exercise, learner)).finally(() => { finished = true; });
// Observe only this fixture's marker; no timing assumption about compiler speed.
// The reference sleeps so its source remains available until the abort arrives.
const deadline = Date.now() + 20_000;
while (!abort.signal.aborted && !finished && Date.now() < deadline) {
  const dirs = (await readdir(tmpdir())).filter((name) => name.startsWith("bm-run-"));
  for (const dir of dirs) {
    const source = await readFile(path.join(tmpdir(), dir, "main.c"), "utf8").catch(() => "");
    if (source.includes(marker)) { abort.abort(); break; }
  }
  if (!abort.signal.aborted) await delay(10);
}
await first.catch(() => null);
assert(abort.signal.aborted, "reference run was observed and its initiating request canceled");
const second = await withRunnerRequest([marker], new AbortController().signal, () => testExercise(exercise, learner));
assert(second.allPassed, `a canceled cold request must not disable later correct submissions: ${second.error}`);
console.log("Cold reference-cache cancellation regression passed.");
