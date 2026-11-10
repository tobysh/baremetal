// Against a local production server; creates and removes only its own test accounts.
import { SESSION_COOKIE } from "../../src/lib/session-cookie";
import assert from "node:assert/strict";
import { createHash, randomBytes } from "node:crypto";
import { MongoClient, ObjectId } from "mongodb";
import { getTracks } from "../../src/lib/curriculum";
import { withRunnerRequest, RunnerBusyError } from "../../src/lib/runner";
const base = process.argv[2] ?? "http://localhost:3100";
const client = await new MongoClient(process.env.MONGODB_URI ?? "mongodb://127.0.0.1:27017").connect();
const db = client.db(process.env.MONGODB_DB ?? "baremetal");
const ids = [new ObjectId(), new ObjectId()];
const tokens = ids.map(() => randomBytes(32).toString("hex"));
const suffix = randomBytes(5).toString("hex");
const ips = [`192.0.2.${100 + Math.floor(Math.random() * 50)}`, `198.51.100.${100 + Math.floor(Math.random() * 50)}`];
const track = getTracks("c-memory")[0];
const ex = track.lessons[0].exercise;
const request = (path: string, body: unknown, user = 0, method = "POST", signal?: AbortSignal) => fetch(base + path, {
  method, signal, headers: { Origin: base, "Content-Type": "application/json", "X-Forwarded-For": ips[user], Cookie: `${SESSION_COOKIE}=${tokens[user]}` },
  ...(method === "DELETE" ? {} : { body: JSON.stringify(body) }),
});
try {
  let release!: () => void;
  const gate = new Promise<void>((resolve) => { release = resolve; });
  const signal = new AbortController().signal;
  const first = withRunnerRequest([suffix], signal, () => gate);
  const second = withRunnerRequest([suffix], signal, () => gate);
  try {
    await assert.rejects(withRunnerRequest([suffix], signal, async () => {}), RunnerBusyError);
    await withRunnerRequest([`${suffix}-other`], signal, async () => {});
  } finally { release(); await Promise.all([first, second]); }
  for (let i = 0; i < ids.length; i++) {
    await db.collection("users").insertOne({ _id: ids[i], email: `security-regression-${suffix}-${i}@example.test`, name: "Security regression", passwordHash: "unused", emailVerified: true, experience: "some", role: "user", sessionVersion: 0, createdAt: new Date(Date.now() - 2 * 86400_000) });
    await db.collection("sessions").insertOne({ userId: ids[i], tokenHash: createHash("sha256").update(tokens[i]).digest("hex"), sessionVersion: 0, createdAt: new Date(), expiresAt: new Date(Date.now() + 3600_000) });
  }
  const quota = () => db.collection("rate_limits").countDocuments({ key: new RegExp(`^tutor:.*${ids[0].toHexString()}`) });
  assert.equal(await quota(), 0);
  for (const body of [{ message: {} }, { message: "help", code: {} }, { message: "help", lastOutput: [] }, { message: "help", code: "a".repeat(16385) }, { message: "help", code: "😀".repeat(5000) }]) {
    assert.equal((await request("/api/tutor", { exerciseId: ex.id, ...body })).status, 400);
  }
  assert.equal(await quota(), 0, "invalid tutor inputs consume no quota");
  const anonHint = await fetch(base + "/api/hint", { method: "POST", headers: { "Content-Type": "application/json", Origin: base }, body: JSON.stringify({ exerciseId: ex.id, index: 0 }) });
  assert.equal(anonHint.status, 401, "hints are not served without a session");
  const hint = await request("/api/hint", { exerciseId: ex.id, index: 0 });
  assert.equal(hint.status, 200);
  assert.deepEqual((await hint.json()).hints, ex.hints.slice(0, 1));
  assert.equal((await db.collection("progress").findOne({ userId: ids[0], exerciseId: ex.id }))?.hintsUsed, 1);
  assert.equal((await request("/api/hint", { exerciseId: ex.id, index: -1 })).status, 400);
  console.log("Tutor validation/quota and server-recorded hint disclosure passed.");

  const slow = { exerciseId: ex.id, mode: "run", code: "#include <unistd.h>\nint main(void) { sleep(2); return 0; }" };
  const flood = Array.from({ length: 30 }, () => request("/api/run", slow));
  const other = await request("/api/run", { exerciseId: ex.id, mode: "run", code: "#include <stdio.h>\nint main(void) { puts(\"available\"); return 0; }" }, 1);
  assert.equal(other.status, 200);
  assert.equal((await other.json()).ok, true);
  const results = await Promise.all(flood);
  assert(results.some((r) => r.status === 200));
  assert(results.some((r) => r.status === 503), "the burst must encounter per-account admission limits");
  assert(results.every((r) => r.status === 200 || r.status === 503));
  console.log("30-request flood constrained to two runs; another account remains available.");
  // A classroom shares one address: signed-in learners are admitted per account, anonymous traffic per IP.
  const classIp = `203.0.113.${1 + Math.floor(Math.random() * 250)}`;
  const crowd = Array.from({ length: 10 }, () => ({ id: new ObjectId(), token: randomBytes(32).toString("hex") }));
  try {
    for (const [i, u] of crowd.entries()) {
      await db.collection("users").insertOne({ _id: u.id, email: `security-regression-${suffix}-class${i}@example.test`, name: "Security regression", passwordHash: "unused", emailVerified: true, experience: "some", role: "user", sessionVersion: 0, createdAt: new Date(Date.now() - 2 * 86400_000) });
      await db.collection("sessions").insertOne({ userId: u.id, tokenHash: createHash("sha256").update(u.token).digest("hex"), sessionVersion: 0, createdAt: new Date(), expiresAt: new Date(Date.now() + 3600_000) });
    }
    const quick = { exerciseId: ex.id, mode: "run", code: "#include <unistd.h>\nint main(void) { sleep(1); return 0; }" };
    const post = (cookie?: string) => fetch(base + "/api/run", { method: "POST", headers: { Origin: base, "Content-Type": "application/json", "X-Forwarded-For": classIp, ...(cookie ? { Cookie: `${SESSION_COOKIE}=${cookie}` } : {}) }, body: JSON.stringify(quick) });
    const signedIn = await Promise.all(crowd.map((u) => post(u.token)));
    assert(signedIn.every((r) => r.status === 200), `ten signed-in learners behind one IP should all be admitted, got ${signedIn.map((r) => r.status)}`);
    const anonymous = await Promise.all(Array.from({ length: 10 }, () => post()));
    assert(anonymous.some((r) => r.status === 503), "anonymous clients behind one IP stay limited");
    assert(anonymous.some((r) => r.status === 200));
    console.log("Ten signed-in learners behind one IP all ran; the anonymous burst from that IP was limited.");
  } finally {
    for (const u of crowd) {
      await db.collection("users").deleteOne({ _id: u.id });
      await db.collection("sessions").deleteMany({ userId: u.id });
      await db.collection("rate_limits").deleteMany({ key: { $regex: u.id.toHexString() } });
    }
  }
  // ---- Authorization, CSRF, headers and stored XSS (Phase 4 of the 2026-11-09 plan) ----
  {
    // Cross-user isolation: the API takes no user id, so another account's data must never appear in any response.
    const secret = `SECRET-${suffix}-user1`;
    assert.equal((await request("/api/progress", { exerciseId: ex.id, code: `// ${secret}\nint main(void){return 0;}` }, 1, "PUT")).status, 200);
    assert.ok(await db.collection("progress").findOne({ userId: ids[1], exerciseId: ex.id, code: new RegExp(secret) }), "user 1 progress saved");
    await db.collection("tutor_threads").updateOne({ userId: ids[1], lessonId: ex.id }, { $set: { messages: [{ role: "user", content: secret, at: new Date() }], updatedAt: new Date() } }, { upsert: true });
    const asZero = (path: string, init: RequestInit = {}) => fetch(base + path, { ...init, headers: { Origin: base, "X-Forwarded-For": ips[0], Cookie: `${SESSION_COOKIE}=${tokens[0]}`, ...(init.headers ?? {}) } });
    const exported = await (await asZero("/api/account/export")).text();
    assert.ok(!exported.includes(secret) && !exported.includes(`-${suffix}-1@example.test`), "export contains only the caller's data");
    for (const path of ["/api/tutor/quota", `/api/tutor?exerciseId=${encodeURIComponent(ex.id)}`, "/api/account/export"]) {
      assert.ok(!(await (await asZero(path)).text()).includes(secret), `${path} leaks no data of another account`);
    }
    for (const body of [{ exerciseId: ex.id, event: "hint", index: 0 }, { exerciseId: ex.id, q: 4 }]) {
      const text = await (await request(body.event ? "/api/progress" : "/api/review", body, 0)).text();
      assert.ok(!text.includes(secret), "progress/review responses carry no other account's code");
    }
    assert.ok(await db.collection("progress").findOne({ userId: ids[1], exerciseId: ex.id, code: new RegExp(secret) }), "user 0 actions did not alter user 1's progress");
    // Another account's shell token and a made-up one are indistinguishable, and anonymous callers are refused.
    const fakeToken = randomBytes(32).toString("base64url");
    assert.ok([401, 404].includes((await asZero(`/api/shell/${fakeToken}`)).status));
    assert.ok([401, 403, 404].includes((await fetch(base + `/api/shell/${fakeToken}`, { method: "DELETE", headers: { Origin: base } })).status));
    for (const path of ["/api/tutor/quota", "/api/account/export"]) assert.equal((await fetch(base + path)).status, 401, `${path} needs a session`);
    console.log("Cross-user isolation passed.");

    // CSRF: every mutating route refuses a missing Origin (no Sec-Fetch-Site), a foreign Origin and a cross-site fetch.
    const mutating: [string, string, unknown][] = [
      ["PUT", "/api/progress", { exerciseId: ex.id, code: "x" }], ["POST", "/api/progress", { exerciseId: ex.id, event: "hint", index: 0 }],
      ["POST", "/api/review", { exerciseId: ex.id, q: 4 }], ["POST", "/api/hint", { exerciseId: ex.id, index: 0 }],
      ["POST", "/api/practice", { exerciseId: ex.id }], ["POST", "/api/run", { exerciseId: ex.id, mode: "run", code: "int main(void){return 0;}" }],
      ["POST", "/api/tutor", { exerciseId: ex.id, message: "help" }], ["DELETE", "/api/tutor?exerciseId=" + encodeURIComponent(ex.id), null],
      ["POST", "/api/shell", {}],
    ];
    const csrf = (method: string, path: string, body: unknown, headers: Record<string, string>) =>
      fetch(base + path, { method, headers: { "Content-Type": "application/json", "X-Forwarded-For": ips[0], Cookie: `${SESSION_COOKIE}=${tokens[0]}`, ...headers }, ...(body === null ? {} : { body: JSON.stringify(body) }) });
    for (const [method, path, body] of mutating) {
      assert.equal((await csrf(method, path, body, {})).status, 403, `${method} ${path} without Origin`);
      assert.equal((await csrf(method, path, body, { Origin: "https://evil.example" })).status, 403, `${method} ${path} with a foreign Origin`);
      assert.equal((await csrf(method, path, body, { "Sec-Fetch-Site": "cross-site" })).status, 403, `${method} ${path} cross-site`);
      assert.equal((await csrf(method, path, body, { Origin: "null" })).status, 403, `${method} ${path} with Origin: null`);
    }
    const beforeEvents = await db.collection("events").countDocuments({});
    await csrf("POST", "/api/analytics", { path: "/" }, { Origin: "https://evil.example" });
    assert.equal(await db.collection("events").countDocuments({}), beforeEvents, "cross-site analytics beacon records nothing");
    console.log("Per-route CSRF checks passed.");

    // Response headers.
    const page = await fetch(base + "/login");
    const csp = page.headers.get("content-security-policy") ?? "";
    for (const directive of ["default-src 'self'", "object-src 'none'", "base-uri 'self'", "form-action 'self'", "frame-ancestors 'none'", "connect-src 'self'"]) assert.ok(csp.includes(directive), `CSP has ${directive}`);
    assert.ok(!/script-src[^;]*(https?:|\*)/.test(csp) && !csp.includes("unsafe-eval"), "no remote script origins or eval in CSP");
    assert.equal(page.headers.get("x-frame-options"), "DENY");
    assert.equal(page.headers.get("x-content-type-options"), "nosniff");
    assert.match(page.headers.get("referrer-policy") ?? "", /strict-origin/);
    assert.equal(page.headers.get("cross-origin-opener-policy"), "same-origin");
    assert.match(page.headers.get("permissions-policy") ?? "", /camera=\(\)/);
    assert.match(page.headers.get("strict-transport-security") ?? "", /max-age=\d{7,}/, "HSTS (run against `next start`, not `next dev`)");
    assert.match((await asZero("/api/tutor/quota")).headers.get("cache-control") ?? "", /no-store/);
    assert.ok(!page.headers.has("x-powered-by"), "no X-Powered-By");
    console.log("CSP and security header assertions passed.");

    // Stored XSS: hostile display values written straight to the database (bypassing signup validation) must render escaped.
    const evil = new ObjectId();
    const payload = `<img src=x onerror=alert(${suffix})>`;
    const username = `xss${suffix}`;
    try {
      await db.collection("users").insertOne({ _id: evil, email: `security-regression-${suffix}-xss@example.test`, name: payload, username, passwordHash: "unused", emailVerified: true, experience: "some", role: "user", sessionVersion: 0, profilePublic: true, xp: 1_000_000, createdAt: new Date(Date.now() - 2 * 86400_000) });
      await db.collection("users").updateOne({ _id: evil }, { $set: { username: payload } });
      for (const path of ["/leaderboard", `/u/${encodeURIComponent(payload)}`, `/u/${username}`]) {
        const html = await (await fetch(base + path)).text();
        assert.ok(!html.includes(payload) && !html.includes("<img src=x onerror"), `${path} escapes stored usernames and names`);
      }
    } finally { await db.collection("users").deleteOne({ _id: evil }); }
    console.log("Stored XSS via username checks passed.");
  }
  for (let i = 0; i < 30; i++) assert.equal((await request(`/api/tutor?exerciseId=${encodeURIComponent(ex.id)}`, null, 0, "DELETE")).status, 200);
  assert.equal((await request(`/api/tutor?exerciseId=${encodeURIComponent(ex.id)}`, null, 0, "DELETE")).status, 429);
  console.log("Tutor conversation deletion rate limit passed.");
} finally {
  for (const name of ["users", "sessions", "progress", "tutor_threads", "xp_events", "achievements", "reviews", "review_events"]) await db.collection(name).deleteMany(name === "users" ? { _id: { $in: ids } } : { userId: { $in: ids } });
  for (const id of ids) await db.collection("rate_limits").deleteMany({ key: { $regex: id.toHexString() } });
  await client.close();
}
