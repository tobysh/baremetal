// Audit checks (items 1 and 3): compile-time reads of the hidden harness that differ between the
// guard build and the real build, and what failure messages disclose.
// Usage: npx tsx scripts/security/audit-checks.mts [probe]
import { getTracks } from "../../src/lib/curriculum/index";
import { testExercise } from "../../src/lib/runner";
import type { Exercise } from "../../src/lib/curriculum/types";

const pick = (lang: string): Exercise | null => {
  for (const t of getTracks()) for (const l of t.lessons) if (l.exercise.language === lang && l.exercise.mode === "harness" && l.exercise.harness) return l.exercise;
  return null;
};
let failed = 0;
const check = (ok: boolean, name: string, detail = "") => { if (!ok) failed++; console.log(`${ok ? "ok  " : "FAIL"} ${name}${detail ? " -> " + detail : ""}`); };

// A leak is any run whose output or messages contain harness text (the tests' result names or CHECK source).
const leaks = (r: Awaited<ReturnType<typeof testExercise>>) => /@@(RUN|PASS|FAIL)|__bm_|BM_NONCE|CHECK\(/.test(`${r.output}\n${r.compileOutput}\n${JSON.stringify(r.results)}`);
const REACH = /could reach the hidden tests|belongs to the test harness/;

// The guard compiles the learner's file in a directory holding only that file and bm_base.c; the real
// build has bm_init.c (C) or runtime.c/boot.S (kernel) beside it. __has_include can tell the two apart,
// so a directive that is only emitted in the real build is never seen by the guard.
const dump = `__attribute__((__constructor__(101))) static void bm_dump(void){ extern const char bm_leak[]; extern const char bm_leak_end[]; write(1, bm_leak, bm_leak_end - bm_leak); }`;
const probes: { lang: "c" | "kernel"; name: string; build: (ex: Exercise) => string }[] = [
  {
    lang: "c", name: "__has_include(bm_init.c) gates a split-string .incbin of main.c",
    build: (ex) => `#include <unistd.h>\n#if __has_include("bm_init.c")\n__asm__(".section .rodata\\n.globl bm_leak\\nbm_leak: .inc" "bin \\"main.c\\"\\n.globl bm_leak_end\\nbm_leak_end: .byte 0\\n.text");\n${dump}\n#endif\n${ex.starter}`,
  },
  {
    // No __has_include: the guard compiles without -fstack-protector-strong, the real build with it.
    lang: "c", name: "#ifdef __SSP_STRONG__ (real-build-only macro) gates a split-string .incbin of main.c",
    build: (ex) => `#include <unistd.h>\n#ifdef __SSP_STRONG__\n__asm__(".section .rodata\\n.globl bm_leak\\nbm_leak: .inc" "bin \\"main.c\\"\\n.globl bm_leak_end\\nbm_leak_end: .byte 0\\n.text");\n${dump}\n#endif\n${ex.starter}`,
  },
  {
    // Observable without a serial channel: the kernel hangs (timeout) iff its own compiled-in copy of kernel.c contains harness text.
    lang: "kernel", name: "__has_include(runtime.c) gates a split-string .incbin of kernel.c (hangs iff harness text is readable)",
    build: (ex) => `#if __has_include("runtime.c")\n__asm__(".section .rodata\\n.globl bm_leak\\nbm_leak: .inc" "bin \\"kernel.c\\"\\n.globl bm_leak_end\\nbm_leak_end: .byte 0\\n.text");\nextern const char bm_leak[]; extern const char bm_leak_end[];\nstatic void bm_probe(void){ const char n[] = "LSR_DATA_READY"; for (const char *p = bm_leak; p + 14 < bm_leak_end; p++) { int k = 0; while (k < 14 && p[k] == n[k]) k++; if (k == 14) for (;;) {} } }\n#endif\n${ex.starter.replace("void serial_init(void) {", "void serial_init(void) {\n#if __has_include(\"runtime.c\")\nbm_probe();\n#endif")}`,
  },
];
// `__has_include` built by token pasting gets past the text scan, so these only stop at the scan of the
// real combined build. Both directions: a file only the real directory held (bm_init.c, runtime.c) and
// one only the standalone guard directory holds (bm_base.c).
const PASTE = "#define BM_P(a, b) a##b\n";
const cLeak = `__asm__(".section .rodata\\n.globl bm_leak\\nbm_leak: .inc" "bin \\"main.c\\"\\n.globl bm_leak_end\\nbm_leak_end: .byte 0\\n.text");\n${dump}\n`;
const kernelLeak = (ex: Exercise, gate: string) => `${PASTE}#if ${gate}\n__asm__(".section .rodata\\n.globl bm_leak\\nbm_leak: .inc" "bin \\"kernel.c\\"\\n.globl bm_leak_end\\nbm_leak_end: .byte 0\\n.text");\nextern const char bm_leak[]; extern const char bm_leak_end[];\nstatic void bm_probe(void){ const char n[] = "LSR_DATA_READY"; for (const char *p = bm_leak; p + 14 < bm_leak_end; p++) { int k = 0; while (k < 14 && p[k] == n[k]) k++; if (k == 14) for (;;) {} } }\n#endif\n${ex.starter.replace("void serial_init(void) {", `void serial_init(void) {\n#if ${gate}\nbm_probe();\n#endif`)}`;
for (const [file, neg] of [["bm_init.c", ""], ["bm_base.c", "!"]] as const) {
  probes.push({ lang: "c", name: `token-pasted ${neg}__has_include(${file}) gates a split-string .incbin of main.c`, build: (ex) => `#include <unistd.h>\n${PASTE}#if ${neg}BM_P(__has_, include)("${file}")\n${cLeak}#endif\n${ex.starter}` });
}
for (const [file, neg] of [["runtime.c", ""], ["bm_base.c", "!"]] as const) {
  probes.push({ lang: "kernel", name: `token-pasted ${neg}__has_include(${file}) gates a split-string .incbin of kernel.c`, build: (ex) => kernelLeak(ex, `${neg}BM_P(__has_, include)("${file}")`) });
}
for (const p of probes) {
  const ex = pick(p.lang);
  if (!ex) continue;
  const r = await testExercise(ex, p.build(ex));
  check(!!r.error && REACH.test(r.error) || !r.compiled || !(leaks(r) || r.timedOut), `[${p.lang}] ${p.name}`, `compiled=${r.compiled} allPassed=${r.allPassed} error=${r.error ?? "-"} leak=${leaks(r)} outlen=${r.output.length} out=${JSON.stringify(r.output.slice(-260))} co=${JSON.stringify(r.compileOutput.slice(0, 300))}`);
}

// ---- Items 2 and 3 need a running server: pass its URL (production mode, runner on RUNNER_SOCKET) and
// MONGODB_DB=<scratch db>. Usage: BM_INSECURE_COOKIES=1 MONGODB_DB=bm_audit_agent npx tsx scripts/security/audit-checks.mts http://localhost:3600
const base = process.argv[2];
if (base) {
  const { createHash, randomBytes } = await import("node:crypto");
  const { MongoClient, ObjectId } = await import("mongodb");
  const { SESSION_COOKIE } = await import("../../src/lib/session-cookie");
  const { HOUR, LIMITS } = await import("../../src/lib/tutor-limits");
  const db = (await new MongoClient(process.env.MONGODB_URI ?? "mongodb://127.0.0.1:27017").connect()).db(process.env.MONGODB_DB ?? "bm_audit_agent");
  if (db.databaseName === "baremetal") throw new Error("Refusing to create audit users in the real database.");
  const suffix = randomBytes(4).toString("hex");
  const ids = [new ObjectId(), new ObjectId()];
  const tokens = ids.map(() => randomBytes(32).toString("hex"));
  const ips = ["192.0.2.201", "198.51.100.202"];
  const call = (path: string, init: { user?: number; body?: unknown; method?: string; ip?: string } = {}) => fetch(base + path, {
    method: init.method ?? (init.body === undefined ? "GET" : "POST"),
    headers: { Origin: process.env.AUDIT_ORIGIN ?? base, "Content-Type": "application/json", "X-Forwarded-For": init.ip ?? (init.user === undefined ? "203.0.113.77" : ips[init.user]), ...(init.user === undefined ? {} : { Cookie: `${SESSION_COOKIE}=${tokens[init.user]}` }) },
    ...(init.body === undefined ? {} : { body: JSON.stringify(init.body) }),
  });
  try {
    for (let i = 0; i < 2; i++) {
      await db.collection("users").insertOne({ _id: ids[i], email: `audit-${suffix}-${i}@example.test`, name: "Audit", passwordHash: "unused", emailVerified: true, experience: "some", role: "user", sessionVersion: 0, createdAt: new Date(Date.now() - 2 * 86400_000) });
      await db.collection("sessions").insertOne({ userId: ids[i], tokenHash: createHash("sha256").update(tokens[i]).digest("hex"), sessionVersion: 0, createdAt: new Date(), expiresAt: new Date(Date.now() + 3600_000) });
    }
    const ex = getTracks("c-memory")[0].lessons[0].exercise;
    const [A, B] = ids.map((i) => i.toHexString());

    // Item 2: /api/tutor/quota and /api/hint act only for the session user; client-supplied ids are ignored.
    check((await call("/api/tutor/quota")).status === 401, "[quota] anonymous GET is refused");
    check((await call("/api/hint", { body: { exerciseId: ex.id, index: 0 } })).status === 401, "[hint] anonymous POST is refused");
    const win = Math.floor(Date.now() / (HOUR * 1000));
    await db.collection("rate_limits").updateOne({ key: `tutor:user:h:${A}:${win}` }, { $set: { count: 7, expiresAt: new Date(Date.now() + 2 * HOUR * 1000) } }, { upsert: true });
    const q = async (user: number, qs = "") => (await call(`/api/tutor/quota${qs}`, { user })).json() as Promise<{ remaining: number }>;
    check((await q(0)).remaining === LIMITS.userPerHour - 7, "[quota] user A sees A's own usage");
    check((await q(1)).remaining === LIMITS.userPerHour, "[quota] user B does not see A's usage");
    check((await q(1, `?userId=${A}&user=${A}&uid=${A}&id=${A}`)).remaining === LIMITS.userPerHour, "[quota] userId/user/uid/id query parameters naming A are ignored for B");
    const hintAs = (user: number, extra: object = {}) => call("/api/hint", { user, body: { exerciseId: ex.id, index: 0, ...extra } });
    check((await hintAs(1, { userId: A, user: A, uid: A })).status === 200, "[hint] B can fetch a hint");
    const hintsUsed = async (id: string) => (await db.collection("progress").findOne({ userId: new ObjectId(id), exerciseId: ex.id }))?.hintsUsed ?? 0;
    check((await hintsUsed(B)) === 1 && (await hintsUsed(A)) === 0, "[hint] a body naming A recorded the hint for B (the session user) only", `A=${await hintsUsed(A)} B=${await hintsUsed(B)}`);
    check((await call("/api/hint?userId=" + A, { user: 1, body: { exerciseId: ex.id, index: 0, review: true } })).status === 200 && (await hintsUsed(A)) === 0, "[hint] review flag with ?userId=A still leaves A untouched");

    // Item 3: what /api/run test mode returns on failure, with and without a session.
    const all = getTracks().flatMap((t) => t.lessons.map((l) => l.exercise));
    const stdoutSets = all.filter((e) => e.mode === "stdout" && e.cases?.some((c) => c.set));
    const stdoutStdin = all.filter((e) => e.mode === "stdout" && e.cases?.some((c) => c.stdin));
    const take = <T,>(list: T[], n: number, key: (x: T) => string) => { const seen = new Map<string, number>(); return list.filter((x) => { const k = key(x), c = seen.get(k) ?? 0; seen.set(k, c + 1); return c < n; }); };
    const sample = [...take(stdoutSets, 1, (e) => e.language), ...take(stdoutStdin, 1, (e) => e.language), ...take(all.filter((e) => e.mode === "harness"), 1, (e) => e.language)];
    const dedup = [...new Map(sample.map((e) => [e.id, e])).values()];
    const lines = (text: string) => text.split("\n").map((l) => l.trim()).filter((l) => l.length >= 30 && !l.startsWith("//") && !l.startsWith("#include"));
    let n = 0;
    for (const e of dedup) {
      const harnessLines = lines(e.harness ?? "");
      const solutionOnly = lines(e.solution).filter((l) => !e.starter.includes(l));
      for (const user of [undefined, 0] as const) {
        const res = await call("/api/run", { user, body: { exerciseId: e.id, mode: "test", code: e.starter }, ip: `203.0.113.${100 + n++}` });
        const text = await res.text();
        if (res.status !== 200) { console.log(`skip [${e.id}] ${user === undefined ? "anon" : "session"} status=${res.status}`); continue; }
        const body = JSON.parse(text) as { results?: { name: string; message?: string; expected?: string }[]; output?: string };
        const withExpected = (body.results ?? []).filter((r) => r.expected !== undefined).length;
        const setCases = (e.cases ?? []).filter((c) => c.set);
        const setLeaked = setCases.filter((c) => c.expected.trim().length > 0 && text.includes(JSON.stringify(c.expected).slice(1, -1))).length;
        const stdinCases = (e.cases ?? []).filter((c) => (c.stdin ?? "").trim().length > 2);
        const stdinLeaked = stdinCases.filter((c) => text.includes(JSON.stringify(c.stdin).slice(1, -1))).length;
        const harnessHit = harnessLines.filter((l) => text.includes(JSON.stringify(l).slice(1, -1))).length;
        const solutionHit = solutionOnly.filter((l) => text.includes(JSON.stringify(l).slice(1, -1))).length;
        console.log(`info [${e.language}/${e.mode}] ${e.id} ${user === undefined ? "anon   " : "session"} cases=${e.cases?.length ?? "-"} results=${body.results?.length ?? 0} expected-returned=${withExpected} set-variant-answers-returned=${setLeaked}/${setCases.length} stdin-values-echoed=${stdinLeaked}/${stdinCases.length} harness-lines=${harnessHit}/${harnessLines.length} solution-lines=${solutionHit}/${solutionOnly.length}`);
        check(withExpected <= 1, `[${e.id}] at most one failing case returns its expected output (got ${withExpected})`);
        check(harnessHit === 0 && solutionHit === 0, `[${e.id}] failure response (${user === undefined ? "anonymous" : "session"}) contains no harness or solution line`);
      }
    }
  } finally {
    for (const id of ids) { await db.collection("users").deleteOne({ _id: id }); await db.collection("sessions").deleteMany({ userId: id }); await db.collection("progress").deleteMany({ userId: id }); await db.collection("rate_limits").deleteMany({ key: { $regex: id.toHexString() } }); }
  }
}
console.log(failed ? `\n${failed} audit check(s) failed.` : "\nAll audit checks passed.");
process.exit(failed ? 1 : 0);
