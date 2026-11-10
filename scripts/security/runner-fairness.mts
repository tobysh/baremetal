// Runner queue fairness with a fake job: no compiler needed.
// Run: RUNNER_COMPILE_SLOTS=1 node --import tsx scripts/security/runner-fairness.mts
import assert from "node:assert/strict";
import { setTimeout as delay } from "node:timers/promises";
import { RunnerBusyError, withRunnerRequest, withRunnerSlot } from "../../src/lib/runner-engine";

const sig = () => new AbortController().signal;
const order: string[] = [];
let concurrentByUser = new Map<string, number>(), maxByUser = new Map<string, number>();
const job = (tag: string, user: string) => async () => {
  const n = (concurrentByUser.get(user) ?? 0) + 1;
  concurrentByUser.set(user, n);
  maxByUser.set(user, Math.max(maxByUser.get(user) ?? 0, n));
  order.push(tag);
  await delay(30);
  concurrentByUser.set(user, n - 1);
};
const as = (user: string, kind: "user" | "ip", fn: () => Promise<void>) => withRunnerRequest([user], sig(), fn, [kind]);

// 1. One slot (RUNNER_COMPILE_SLOTS=1). A greedy account queues 2 compiles before a second account asks.
//    FIFO would run a1, a2, b1; fair service runs a1, b1, a2.
const a = as("acct-a", "user", async () => { await withRunnerSlot("compile", job("a1", "acct-a")); });
const a2 = as("acct-a", "user", async () => { await withRunnerSlot("compile", job("a2", "acct-a")); });
await delay(5);
const b = as("acct-b", "user", async () => { await withRunnerSlot("compile", job("b1", "acct-b")); });
await Promise.all([a, a2, b]);
assert.deepEqual(order, ["a1", "b1", "a2"], `fair order, got ${order}`);

// 2. A user never holds two compile slots, even with spare capacity (set-case recompiles are sequential admissions).
if (Number(process.env.RUNNER_COMPILE_SLOTS) >= 2) {
  concurrentByUser = new Map(); maxByUser = new Map();
  await Promise.all([1, 2].map((i) => as("acct-c", "user", () => withRunnerSlot("compile", job(`c${i}`, "acct-c")))));
  assert.equal(maxByUser.get("acct-c"), 1, "one compile slot per account");
}

// 3. Compile budget: every compile counts.
await assert.rejects(async () => {
  for (let i = 0; i < 200; i++) await as("acct-d", "user", () => withRunnerSlot("compile", async () => {}));
}, RunnerBusyError);
console.log("Runner fairness checks passed.");
