// Run with: node --conditions=react-server --import tsx scripts/tutor/cost-checks.mts   (no API key or DB needed)
import assert from "node:assert/strict";
import { buildMessages, conceptCacheKey, failureCacheKey, lessonContext, planCache, responseCacheKey } from "../../src/lib/tutor";
import { classifyQuestion, codeShape, expiryFor, normalizeFailureOutput, normalizeQuestion, privateWords, replyLeaks, wordsOf, POPULAR_HITS } from "../../src/lib/tutor-cache";
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
// --- shared caches: classification
for (const m of ["What is a pointer?", "hey, can you explain what's a pointer??", "How do I loop over an array", "Why use a struct instead of separate variables?"]) assert.equal(classifyQuestion(m), "concept", m);
for (const m of ["Why does my code crash?", "what does this line do", "how do I solve this exercise", "I don't know where to start", "why is line 4 wrong", "x = malloc(4); what is that", "what is a pointer? my code: int *p;"])
  assert.notEqual(classifyQuestion(m), "concept", m);
for (const m of ["I don't know where to start", "Help!", "I'm stuck.", "what does this error mean?", "Why does it fail?"]) assert.equal(classifyQuestion(m), "generic", m);
assert.equal(normalizeQuestion("Hey, can you explain what's a pointer??"), normalizeQuestion("what is a pointer"));

// --- concept key ignores code; differs by lesson context and wording
assert.equal(conceptCacheKey("L1", "What is a pointer?"), conceptCacheKey("L1", "please what's a pointer"));
assert.notEqual(conceptCacheKey("L1", "What is a pointer?"), conceptCacheKey("L2", "What is a pointer?"));
assert.notEqual(conceptCacheKey("L1", "What is a pointer?"), conceptCacheKey("L1", "What is an array?"));
const plan = (message: string, code: string, lastOutput = "", lessonCtx = "ctx pointer array", starter = "int main(void) { return 0; }") =>
  planCache({ lessonCtx, message, code, lastOutput, language: "c", starter });
const pc1 = plan("What is a pointer?", "int secret_ident = 4; // my private note");
const pc2 = plan("what is a pointer", "completely different code");
assert.equal(pc1.omitContext, true);
assert.deepEqual(pc1.candidates.map((c) => c.key), pc2.candidates.map((c) => c.key), "concept plans do not depend on code");
assert.equal(pc1.candidates[0].priv.size, 0);
assert.equal(buildMessages([], "What is a pointer?", "int secret_ident = 4;", "out", true).at(-1)!.content.includes("secret_ident"), false, "concept prompt never carries code");
assert.equal(buildMessages([], "q", "int secret_ident = 4;", "OUTMARK", true).at(-1)!.content.includes("OUTMARK"), false);

// --- failure cache: same failure + same shape (renamed vars, comments) shares; different output/shape does not
const pub = wordsOf("ctx pointer array int main void return");
const fa = `int main(void) {\n  int total = 0; // sum\n  for (int i = 0; i <= 10; i++) total += i;\n  return total;\n}`;
const fb = `int main(void) {\n  int acc = 0;\n  for (int k = 0; k <= 10; k++) acc += k;\n  return acc;\n}`;
const fc = fb.replace("<= 10", "< 10");
const fk = (code: string, lastOutput = "got 55\nexpected 45") => failureCacheKey({ lessonCtx: "ctx", code, lastOutput, language: "c", publicWords: pub });
assert.equal(codeShape(fa, "c", pub), codeShape(fb, "c", pub));
assert.equal(fk(fa), fk(fb));
assert.notEqual(fk(fa), fk(fc), "a different condition is a different shape");
assert.notEqual(fk(fa), fk(fa, "got 56\nexpected 45"), "different output");
assert.notEqual(failureCacheKey({ lessonCtx: "other", code: fa, lastOutput: "x", language: "c", publicWords: pub }), failureCacheKey({ lessonCtx: "ctx", code: fa, lastOutput: "x", language: "c", publicWords: pub }));
assert.notEqual(codeShape("int *p;", "c", pub), codeShape("char *p;", "c", pub), "type keywords are kept");
assert.notEqual(codeShape("mov rax, 1", "asm", pub), codeShape("mov rbx, 1", "asm", pub), "assembly is not abstracted");
assert.equal(normalizeFailureOutput("SEGV at 0x7ffd12345678 in /tmp/abc123/main (0.02s)"), "SEGV at 0xADDR in /PATH (TIME)");
const pg = plan("I'm stuck", fa, "got 55");
assert.deepEqual(pg.candidates.map((c) => c.kind), ["exact", "failure"]);
assert.equal(pg.omitContext, false);
assert.deepEqual(plan("why is my loop wrong at i==10", fa).candidates.map((c) => c.kind), ["exact"], "specific questions only use the exact cache");

// --- privacy: a reply that repeats the author's private words is not shared
const authorWords = privateWords(fa, "failure", "c", pub);
assert(authorWords.has("total"), [...authorWords].join());
assert(replyLeaks("Your total starts at zero.", authorWords));
assert(!replyLeaks("The loop runs one time too many because of the comparison.", authorWords));
assert.equal(plan("I'm stuck", fa, "out").candidates.find((c) => c.kind === "failure")!.storable, false, "unvetted comment ('sum') blocks sharing");
assert.equal(plan("I'm stuck", fb, "out").candidates.find((c) => c.kind === "failure")!.storable, true);
assert(privateWords("int main(void){ // call mom at 555\n return 0;}", "exact", "c", pub).has("mom"), "exact-cache replies must not repeat comment text");
assert(!privateWords("int total = 0;", "exact", "c", pub).has("total"), "identical code words are not private for the exact cache");
// exact keys are unchanged by the new code
assert.equal(responseCacheKey({ lessonCtx: "c", message: "m", code: "x", lastOutput: "", language: "c" }), responseCacheKey({ lessonCtx: "c", message: "M!", code: "x ", lastOutput: "", language: "c" }));

// --- popularity: longer TTL once asked often
const t0 = Date.parse("2026-10-29T00:00:00Z");
assert.equal(Math.round((expiryFor(1, t0).getTime() - t0) / 86_400_000), 30);
assert.equal(Math.round((expiryFor(POPULAR_HITS, t0).getTime() - t0) / 86_400_000), 90);

// --- hits cost nothing: budget helpers only ever spend through recordSpend (never called for cache hits in the route)
console.log("Tutor cache-key, context-size and budget checks passed.");
