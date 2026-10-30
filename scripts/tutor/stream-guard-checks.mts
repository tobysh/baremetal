// Run with: node --conditions=react-server --import tsx scripts/tutor/stream-guard-checks.mts   (no API key or DB needed)
// A streamed reply must never reveal a 24-token solution excerpt, however it is chunked.
import assert from "node:assert/strict";
import { exposesSolution, SolutionGuard } from "../../src/lib/tutor";

const solution = `int sum(const int *a, int n) {\n  int total = 0;\n  for (int i = 0; i < n; i++) { total += a[i]; }\n  return total;\n}\n`;
const starter = `int sum(const int *a, int n) {\n  return 0;\n}\n`;
const excerpt = "for (int i = 0; i < n; i++) { total += a[i]; } return total; }"; // well over 24 tokens of the solution
const leaking = `Here is the idea. First set up a counter. Then use this loop: ${excerpt} That is all you need.`;
const clean = "Think about what the function must return when the array is empty, then about how a running total changes as you look at each element in turn.";

function stream(reply: string, chunk: number) {
  const guard = new SolutionGuard(solution, starter);
  let shown = "";
  for (let i = 0; i < reply.length; i += chunk) shown += guard.push(reply.slice(i, i + chunk));
  shown += guard.finish();
  return { guard, shown };
}

for (const chunk of [1, 2, 3, 5, 7, 16, 64, 1000]) {
  const a = stream(clean, chunk);
  assert.equal(a.guard.leaked, false);
  assert.equal(a.shown, clean, `a clean reply streams intact (chunks of ${chunk})`);

  const b = stream(leaking, chunk);
  assert.equal(b.guard.leaked, true, `the excerpt is detected (chunks of ${chunk})`);
  assert.equal(exposesSolution(leaking, solution, starter), true);
  assert.equal(exposesSolution(b.shown, solution, starter), false, "what reached the client contains no 24-token excerpt");
  assert(!b.shown.includes("total += a[i]; } return total"), "the end of the excerpt never reaches the client");
  assert.equal(b.guard.released, b.shown, "released text is exactly what was shown");
}

// A reply that merely contains the given starter is not a leak.
assert.equal(stream(`Your starter is: ${starter}`, 4).guard.leaked, false);
console.log("Stream guard checks passed.");
