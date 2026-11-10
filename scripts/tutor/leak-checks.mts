// Run with: node --conditions=react-server --import tsx scripts/tutor/leak-checks.mts   (no API key or DB needed)
// Solution leaks that dodge a plain token comparison, and Ollama failures that must not reach learners.
import assert from "node:assert/strict";
process.env.TUTOR_PROVIDER = "ollama";
process.env.OLLAMA_HOST = "http://secret-host.internal:11434";
process.env.OLLAMA_TIMEOUT_MS = "80";
const { exposesSolution, SolutionGuard, streamTutor, TutorUnavailableError } = await import("../../src/lib/tutor");

const solution = `int sum(const int *a, int n) {\n  int total = 0;\n  for (int i = 0; i < n; i++) { total += a[i]; }\n  return total;\n}\n`;
const starter = `int sum(const int *a, int n) {\n  return 0;\n}\n`;
const body = "for (int i = 0; i < n; i++) { total += a[i]; } return total; }";
const wrap = (s: string) => `Try this:\n\`\`\`c\n${s}\n\`\`\``;

function streamed(reply: string, prior: string[] = [], chunk = 5) {
  const g = new SolutionGuard(solution, starter, prior);
  let shown = "";
  for (let i = 0; i < reply.length; i += chunk) shown += g.push(reply.slice(i, i + chunk));
  shown += g.finish();
  return { leaked: g.leaked, shown };
}

// Evasions: each must be flagged by both exposesSolution and the streaming guard.
const evasions: Record<string, string> = {
  "renamed identifiers": "for (int k = 0; k < count; k++) { acc += items[k]; } return acc; }",
  "block comments between tokens": "for /* loop */ (int i = 0; i < n; /* step */ i++) { total += /* add */ a[i]; } return total; }",
  "line comments between tokens": "for (int i = 0; // start\n i < n; i++) { // body\n total += a[i]; } return total; }",
  "zero-width characters": body.replaceAll(" ", "​ ").replace("total", "to​tal"),
  "non-ASCII filler": body.replaceAll(";", ";é").replace("return", "re­turn"),
  "fullwidth letters": body.replace("for", "ｆｏｒ"),
};
for (const [name, reply] of Object.entries(evasions)) {
  assert.equal(exposesSolution(wrap(reply), solution, starter), true, `exposesSolution: ${name}`);
  const s = streamed(wrap(reply));
  assert.equal(s.leaked, true, `guard: ${name}`);
}

// Assembly: `;` and `#` comments interleaved between instructions must not hide the solution.
{
  const asmSolution = "sum:\n  xor eax, eax\n  xor ecx, ecx\n.loop:\n  cmp ecx, esi\n  jge .done\n  add eax, [rdi + rcx*4]\n  inc rcx\n  jmp .loop\n.done:\n  ret\n";
  const asmStarter = "sum:\n  xor eax, eax\n  ret\n";
  const lines = asmSolution.trim().split("\n");
  for (const [name, mark] of [["semicolon", "; note"], ["hash", "# note"]] as const) {
    const reply = "```asm\n" + lines.map((l) => `${l} ${mark} x`).join("\n") + "\n```";
    assert.equal(exposesSolution(reply, asmSolution, asmStarter, [], "asm"), true, `asm ${name} comments: exposesSolution`);
    const g = new SolutionGuard(asmSolution, asmStarter, [], "asm");
    for (let i = 0; i < reply.length; i += 5) g.push(reply.slice(i, i + 5));
    g.finish();
    assert.equal(g.leaked, true, `asm ${name} comments: guard`);
  }
  assert.equal(exposesSolution("Loop over the array adding each element to eax; return it.", asmSolution, asmStarter, [], "asm"), false);
}

// A solution wrapped in an unterminated comment is still visible to the learner.
assert.equal(exposesSolution(`/* ${body}`, solution, starter), true, "unterminated comment");

// Split across turns: neither half alone is 24 tokens of the solution.
const half = body.indexOf("total +=");
const [first, second] = [body.slice(0, half), body.slice(half)];
assert.equal(exposesSolution(first, solution, starter), false);
assert.equal(exposesSolution(second, solution, starter), false);
assert.equal(exposesSolution(second, solution, starter, [first]), true, "split across turns");
const sp = streamed(second, [first]);
assert.equal(sp.leaked, true, "guard sees earlier assistant replies");
assert(!sp.shown.includes("return total; }"), "the completing text is not shown");
assert.equal(streamed("Think about the empty array.", [first]).leaked, false);

// Very short solutions are still protected.
const tiny = "int dbl(int x) { return x * 2 + 1; }";
assert.equal(exposesSolution(`Use: ${tiny}`, tiny, "int dbl(int x) { return 0; }"), true, "short solution");
assert.equal(exposesSolution("Use x * 2 and then add one.", tiny, "int dbl(int x) { return 0; }"), false);

// No false positives on ordinary help, the starter, or prose.
assert.equal(exposesSolution("Think about what happens when n is 0, then how a running total changes per element.", solution, starter), false);
assert.equal(exposesSolution(starter, solution, starter), false);
assert.equal(streamed(`Your starter is: ${starter}`).leaked, false);

// Ollama failures: no host, env var names, commands or raw errors for the learner.
const keepAlive = setInterval(() => {}, 1000); // AbortSignal.timeout timers are unref'd
async function failure(fetchImpl: typeof fetch): Promise<Error> {
  const orig = globalThis.fetch;
  globalThis.fetch = fetchImpl;
  try {
    const it = streamTutor("ctx", [{ role: "user", content: "hi" }], new AbortController().signal, () => {});
    for await (const _ of it) void _;
  } catch (e) {
    return e as Error;
  } finally {
    globalThis.fetch = orig;
  }
  assert.fail("expected an error");
}
const lines = (s: string) => new Response(new ReadableStream({ start(c) { c.enqueue(new TextEncoder().encode(s)); c.close(); } }));
const cases: Record<string, typeof fetch> = {
  "connection refused": async () => { throw new Error("connect ECONNREFUSED secret-host.internal:11434"); },
  "missing model": async () => new Response("model 'x' not found", { status: 404 }),
  "server error": async () => new Response("boom at secret-host.internal", { status: 500 }),
  "error chunk": async () => lines(`{"error":"CUDA OOM at /srv/ollama/secret-host.internal"}\n`),
  "malformed json": async () => lines(`{"message":{"content":"hi"}}\nnot json at secret-host\n`),
  "stalled": ((_u: unknown, init?: RequestInit) => new Promise((_, rej) => init?.signal?.addEventListener("abort", () => rej(new Error("aborted secret-host"))))) as typeof fetch,
};
for (const [name, impl] of Object.entries(cases)) {
  const e = await failure(impl);
  assert(e instanceof TutorUnavailableError, `${name}: learner-safe error type (got ${e.constructor.name}: ${e.message})`);
  assert(!/secret-host|OLLAMA|ollama|ECONN|CUDA|\/srv|11434|\.env/i.test(e.message), `${name}: no internals in "${e.message}"`);
}
clearInterval(keepAlive);
console.log("Leak checks passed.");
