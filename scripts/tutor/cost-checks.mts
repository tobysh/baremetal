// Run with: node --conditions=react-server --import tsx scripts/tutor/cost-checks.mts   (no API key or DB needed)
import assert from "node:assert/strict";
import { buildMessages, lessonContext, responseCacheKey } from "../../src/lib/tutor";
import { normalizeCode, normalizeMessage, normalizeOutput } from "../../src/lib/tutor-normalize";
import { budgetDay, checkBudget, estimateCostMicro, recordSpend, type SpendDb } from "../../src/lib/tutor-budget";
import { getTracks } from "../../src/lib/curriculum";

// --- cache-key normalisation
const a = `int main(void) {\n    // say hi\n    printf("a  b // not a comment");   /* x */\n    return 0;\n}\n`;
const b = `int main(void){\r\n\r\n\r\n  printf("a  b // not a comment");\n  return 0; // done\n}`;
const ctx = "ctx";
const key = (code: string, o = { message: "Why does this crash?", lastOutput: "boom\n", language: "c" }) => responseCacheKey({ lessonCtx: ctx, code, ...o });
assert.equal(normalizeCode(a, "c").includes("a  b // not a comment"), true, "string literal kept verbatim");
assert.equal(key(a), key(b));
assert.notEqual(key(a), key(b.replace("a  b", "a b")), "literal change changes key");
assert.notEqual(key(a), key(a.replace("return 0", "return 1")));
assert.equal(key(a), key(a, { message: "  why does this CRASH ", lastOutput: "boom   \r\n\r\n", language: "c" }));
assert.equal(normalizeOutput("\u001b[31merr\u001b[0m  \n\n\n\nx\n"), "err\n\nx");
assert.equal(normalizeMessage("Help me!!  "), "help me");
assert.equal(normalizeCode("mov rax, 1 # one\n\n   ret", "asm"), "mov rax,1\nret");
assert.equal(normalizeCode("let s = 'a; // c", "rust"), "let s='a;");
assert.equal(normalizeCode("x := `a // b` // c", "go"), "x:=`a // b`");

// --- trimmed context
const track = getTracks("c-memory")[0];
const lesson = track.lessons[0];
const lc = lessonContext(track, lesson, "some");
assert(!lc.includes(lesson.exercise.solution.trim()));
assert(lc.length < 3500 + 1500 + 4 * 200 * 2 + 1500, `lesson context too large: ${lc.length}`);
const big = buildMessages(Array.from({ length: 20 }, (_, i) => ({ role: i % 2 ? "assistant" : "user", content: "x".repeat(5000), at: new Date() })) as never, "hi", "y".repeat(50000), "z".repeat(50000));
assert(big.length <= 7);
assert(big.at(-1)!.content.length < 6000 + 1500 + 300);

// --- cost + budget
process.env.TUTOR_BUDGET_USER_DAY_USD = "0.01";
process.env.TUTOR_BUDGET_GLOBAL_DAY_USD = "0.015";
const usage = { inputTokens: 1000, outputTokens: 200, cacheReadTokens: 5000, cacheWriteTokens: 0 };
assert.equal(estimateCostMicro(usage), 1000 + 500 + 1000); // $0.0025
assert.equal(estimateCostMicro({ ...usage, provider: "ollama" }), 0);
const store = new Map<string, number>();
const db: SpendDb = {
  collection: () => ({
    findOne: async ({ key }) => (store.has(key) ? { micro: store.get(key) } : null),
    updateOne: async ({ key }, u) => void store.set(key, (store.get(key) ?? 0) + u.$inc.micro),
  }),
};
const now = Date.parse("2026-10-29T12:00:00Z");
assert.equal((await checkBudget(db, "u1", now)).ok, true);
for (let i = 0; i < 4; i++) await recordSpend(db, "u1", usage, now); // $0.01
const v = await checkBudget(db, "u1", now);
assert(!v.ok && v.scope === "user" && /12 hours/.test(v.message), JSON.stringify(v));
assert.equal((await checkBudget(db, "u2", now)).ok, true, "other users unaffected");
for (let i = 0; i < 2; i++) await recordSpend(db, "u2", usage, now); // global now $0.015
const g = await checkBudget(db, "u3", now);
assert(!g.ok && g.scope === "global");
assert.equal((await checkBudget(db, "u1", now + 86_400_000)).ok, true, "resets next UTC day");
assert.equal(budgetDay(now), "2026-10-29");
process.env.TUTOR_BUDGET_USER_DAY_USD = "0";
process.env.TUTOR_BUDGET_GLOBAL_DAY_USD = "0";
assert.equal((await checkBudget(db, "u1", now)).ok, true, "0 disables");
console.log("Tutor cache-key, context-size and budget checks passed.");
