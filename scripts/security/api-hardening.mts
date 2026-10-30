// Against a local production server; creates and removes only its own test accounts.
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
  method, signal, headers: { Origin: base, "Content-Type": "application/json", "X-Forwarded-For": ips[user], Cookie: `bm_session=${tokens[user]}` },
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
    const post = (cookie?: string) => fetch(base + "/api/run", { method: "POST", headers: { Origin: base, "Content-Type": "application/json", "X-Forwarded-For": classIp, ...(cookie ? { Cookie: `bm_session=${cookie}` } : {}) }, body: JSON.stringify(quick) });
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
  for (let i = 0; i < 30; i++) assert.equal((await request(`/api/tutor?exerciseId=${encodeURIComponent(ex.id)}`, null, 0, "DELETE")).status, 200);
  assert.equal((await request(`/api/tutor?exerciseId=${encodeURIComponent(ex.id)}`, null, 0, "DELETE")).status, 429);
  console.log("Tutor conversation deletion rate limit passed.");
} finally {
  for (const name of ["users", "sessions", "progress", "tutor_threads", "xp_events", "achievements", "reviews"]) await db.collection(name).deleteMany(name === "users" ? { _id: { $in: ids } } : { userId: { $in: ids } });
  for (const id of ids) await db.collection("rate_limits").deleteMany({ key: { $regex: id.toHexString() } });
  await client.close();
}
