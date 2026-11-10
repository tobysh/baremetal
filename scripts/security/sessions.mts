// Session rotation, grace, idle and admin limits against the real auth.ts and a throwaway database.
// Run: npm run test:sessions   (needs MongoDB; uses MONGODB_DB=bm_tests_agent unless set)
import test, { mock, after } from "node:test";
import assert from "node:assert/strict";
import { AsyncLocalStorage } from "node:async_hooks";
import { spawnSync } from "node:child_process";
import { ObjectId } from "mongodb";

process.env.MONGODB_DB ??= "bm_tests_agent";
if (process.env.MONGODB_DB === "baremetal") throw new Error("refusing to run against the real database");
delete process.env.BM_INSECURE_COOKIES;

const jars = new AsyncLocalStorage<Map<string, string>>();
const fallback = new Map<string, string>();
const currentJar = () => jars.getStore() ?? fallback;
// @types/node predates the `exports` option of mock.module (Node 22.3+), so go through a loose type.
const mockModule = mock.module.bind(mock) as unknown as (id: string, options: { exports: Record<string, unknown> }) => void;
mockModule("next/headers", {
  exports: {
    cookies: async () => {
      const m = currentJar();
      return { get: (n: string) => (m.has(n) ? { name: n, value: m.get(n)! } : undefined), set: (n: string, v: string) => void m.set(n, v), delete: (n: string) => void m.delete(n) };
    },
  },
});
mockModule("next/cache", { exports: { io: async () => {} } });
mockModule("next/navigation", { exports: { redirect: () => { throw new Error("redirect"); } } });

const auth = await import("../../src/lib/auth");
const { getDb } = await import("../../src/lib/db");
const { sha256 } = await import("../../src/lib/crypto");
const { SESSION_COOKIE } = await import("../../src/lib/session-cookie");
const db = await getDb();
const H = 3_600_000;
const ago = (ms: number) => new Date(Date.now() - ms);
const created: ObjectId[] = [];

async function mkUser(role: "user" | "admin" = "user") {
  const _id = new ObjectId();
  created.push(_id);
  await db.collection("users").insertOne({ _id, email: `sess-${_id.toHexString()}@example.test`, name: "t", passwordHash: "x", emailVerified: true, experience: "some", role, sessionVersion: 0, createdAt: new Date() });
  return { _id, sessionVersion: 0, role };
}
/** Creates a session through the real helper and returns its token and a jar holding it. */
async function login(user: Awaited<ReturnType<typeof mkUser>>) {
  const jar = new Map<string, string>();
  await jars.run(jar, () => auth.createSession(user));
  return { jar, token: jar.get(SESSION_COOKIE)! };
}
const patch = (token: string, set: object) => db.collection("sessions").updateOne({ tokenHash: sha256(token) }, { $set: set });
const loads = (jar: Map<string, string>) => jars.run(jar, () => auth.getUserDoc());
const rotate = (jar: Map<string, string>) => jars.run(jar, () => auth.rotateSessionIfDue());
const count = (user: { _id: ObjectId }) => db.collection("sessions").countDocuments({ userId: user._id });

after(async () => {
  await db.collection("sessions").deleteMany({ userId: { $in: created } });
  await db.collection("users").deleteMany({ _id: { $in: created } });
  process.exit(0);
});

test("createSession: 30 days for users, 12 hours for admins, cookie is set", async () => {
  const u = await mkUser();
  const a = await mkUser("admin");
  const su = await login(u);
  const sa = await login(a);
  const exp = async (t: string) => (await db.collection("sessions").findOne({ tokenHash: sha256(t) }))!.expiresAt.getTime() - Date.now();
  assert.ok(Math.abs((await exp(su.token)) - 30 * 24 * H) < 10_000);
  assert.ok(Math.abs((await exp(sa.token)) - 12 * H) < 10_000);
});

test("no rotation while the token is younger than 24 hours", async () => {
  const u = await mkUser();
  const s = await login(u);
  await patch(s.token, { createdAt: ago(23 * H) });
  await rotate(s.jar);
  assert.equal(s.jar.get(SESSION_COOKIE), s.token);
  assert.equal(await count(u), 1);
});

test("rotation after 24h keeps the old token valid for a 60 s grace, then it expires", async () => {
  const u = await mkUser();
  const s = await login(u);
  await patch(s.token, { createdAt: ago(25 * H) });
  const old = s.token;
  await rotate(s.jar);
  const next = s.jar.get(SESSION_COOKIE)!;
  assert.notEqual(next, old, "cookie holds a new token");
  assert.equal(await count(u), 2);
  const oldDoc = (await db.collection("sessions").findOne({ tokenHash: sha256(old) }))!;
  assert.equal(oldDoc.retired, true);
  const left = oldDoc.expiresAt.getTime() - Date.now();
  assert.ok(left > 50_000 && left <= 60_000, `grace is about 60 s, got ${left} ms`);
  assert.ok(await loads(new Map([[SESSION_COOKIE, old]])), "old token still works in the grace window");
  assert.ok(await loads(new Map([[SESSION_COOKIE, next]])), "new token works");
  assert.equal(await jars.run(new Map([[SESSION_COOKIE, old]]), () => auth.rotateSessionIfDue()), undefined);
  assert.equal(await count(u), 2, "a retired token cannot rotate again");
  await patch(old, { expiresAt: ago(1000) });
  assert.equal(await loads(new Map([[SESSION_COOKIE, old]])), null, "old token is dead once the grace ends");
  // The new record is not due again until another 24 h pass (rotatedAt, not createdAt, decides).
  await rotate(s.jar);
  assert.equal(s.jar.get(SESSION_COOKIE), next);
  await patch(next, { rotatedAt: ago(25 * H) });
  await rotate(s.jar);
  assert.notEqual(s.jar.get(SESSION_COOKIE), next, "rotates again 24 h after the last rotation");
});

test("only one of two racing rotations wins", async () => {
  const u = await mkUser();
  const s = await login(u);
  await patch(s.token, { createdAt: ago(48 * H) });
  const jarA = new Map([[SESSION_COOKIE, s.token]]);
  const jarB = new Map([[SESSION_COOKIE, s.token]]);
  await Promise.all([rotate(jarA), rotate(jarB)]);
  const changed = [jarA, jarB].filter((j) => j.get(SESSION_COOKIE) !== s.token);
  assert.equal(changed.length, 1, "exactly one request received a new token");
  assert.equal(await count(u), 2, "exactly one successor record");
});

test("logout removes the retired sibling along with the current record", async () => {
  const u = await mkUser();
  const s = await login(u);
  await patch(s.token, { createdAt: ago(25 * H) });
  await rotate(s.jar);
  assert.equal(await count(u), 2);
  await jars.run(s.jar, () => auth.destroySession());
  assert.equal(await count(u), 0, "no sibling survives logout");
  assert.equal(s.jar.has(SESSION_COOKIE), false);
});

test("users are signed out after 14 days idle, not before", async () => {
  const u = await mkUser();
  const s = await login(u);
  await patch(s.token, { lastSeenAt: ago(13 * 86_400_000) });
  assert.ok(await loads(s.jar));
  await patch(s.token, { lastSeenAt: ago(15 * 86_400_000) });
  assert.equal(await loads(s.jar), null);
});

test("admins: 2 hour idle limit and 12 hour absolute limit", async () => {
  const a = await mkUser("admin");
  const s = await login(a);
  await patch(s.token, { lastSeenAt: ago(1 * H) });
  assert.ok(await loads(s.jar), "1 h idle is fine");
  await patch(s.token, { lastSeenAt: ago(3 * H) });
  assert.equal(await loads(s.jar), null, "3 h idle is too long");
  await patch(s.token, { lastSeenAt: new Date(), createdAt: ago(11 * H) });
  assert.ok(await loads(s.jar), "11 h old and active is fine");
  await patch(s.token, { lastSeenAt: new Date(), createdAt: ago(13 * H), expiresAt: new Date(Date.now() + 5 * H) });
  assert.equal(await loads(s.jar), null, "past 12 h is dead even if active and unexpired");
  // A user promoted to admin later gets the admin limits on their older session.
  const u = await mkUser();
  const su = await login(u);
  await patch(su.token, { lastSeenAt: ago(3 * H) });
  assert.ok(await loads(su.jar));
  await db.collection("users").updateOne({ _id: u._id }, { $set: { role: "admin" } });
  await patch(su.token, { lastSeenAt: ago(3 * H) }); // the load above refreshed it
  assert.equal(await loads(su.jar), null);
});

test("cookie names: __Host- prefix and Secure by default, plain names only with BM_INSECURE_COOKIES=1", () => {
  const probe = (flag?: string) => {
    const env = { ...process.env } as NodeJS.ProcessEnv;
    delete env.BM_INSECURE_COOKIES;
    if (flag !== undefined) env.BM_INSECURE_COOKIES = flag;
    const r = spawnSync(process.execPath, ["--import", "tsx", "-e", `import("./src/lib/session-cookie.ts").then(m=>console.log(JSON.stringify([m.SESSION_COOKIE,m.PENDING_2FA_COOKIE,m.SECURE_COOKIES])))`], { env, encoding: "utf8" });
    assert.equal(r.status, 0, r.stderr);
    return JSON.parse(r.stdout.trim().split("\n").pop()!);
  };
  assert.deepEqual(probe(), ["__Host-bm_session", "__Host-bm_2fa_pending", true]);
  assert.deepEqual(probe("0"), ["__Host-bm_session", "__Host-bm_2fa_pending", true], "only the exact value 1 opts out");
  assert.deepEqual(probe("1"), ["bm_session", "bm_2fa_pending", false]);
});
