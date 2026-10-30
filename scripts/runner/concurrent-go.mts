// Eight Go Tests at once: each must pass or be turned away as busy/queued. A failure from memory, process
// or address-space limits means compile concurrency exceeds what the host can hold (docs/deployment-hardening.md).
import { getTracks } from "../../src/lib/curriculum/index";
import { RunnerBusyError, testExercise, withRunnerRequest } from "../../src/lib/runner";

const ex = getTracks("go-foundations")[0].lessons.find((l) => l.exercise.mode === "harness")!.exercise;
const outcomes = await Promise.all(Array.from({ length: 8 }, (_, i) =>
  withRunnerRequest([`concurrent-go-${i}-${Date.now()}`], new AbortController().signal, () => testExercise(ex, ex.solution))
    .then((r) => ({ kind: r.allPassed ? "passed" : "failed", detail: r.error ?? r.compileOutput.slice(0, 300) }))
    .catch((e) => ({ kind: e instanceof RunnerBusyError ? "busy" : "error", detail: String(e) }))));
for (const o of outcomes) console.log(`  ${o.kind}${o.kind === "passed" || o.kind === "busy" ? "" : ` — ${o.detail}`}`);
const bad = outcomes.filter((o) => o.kind === "failed" || o.kind === "error");
console.log(bad.length ? `\n${bad.length} job(s) failed under concurrency.` : "\nNo job failed with a resource error.");
process.exit(bad.length || !outcomes.some((o) => o.kind === "passed") ? 1 : 0);
