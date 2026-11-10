// Rate-limit checks against a running server: /api/account/export, /api/shell and /api/analytics.
// Usage: tsx --conditions=react-server --env-file-if-exists=.env.local scripts/security/rate-limits.mts http://localhost:3100 [origin]
// `origin` is the Origin header to send when the server's configured site URL differs from the base URL
// (production compares Origin with NEXT_PUBLIC_SITE_URL as it was at build time). Uses its own users, IPs and
// events, and removes them afterwards. Run it against the same database the server uses.
import assert from "node:assert/strict";
import { createHash, randomBytes } from "node:crypto";
import { MongoClient, ObjectId } from "mongodb";
import { SESSION_COOKIE } from "../../src/lib/session-cookie";
import { CONSENT_COOKIE, serializeConsent } from "../../src/lib/consent";

const base = (process.argv[2] ?? "http://localhost:3100").replace(/\/$/, "");
const origin = process.argv[3] ?? base;
const client = await new MongoClient(process.env.MONGODB_URI ?? "mongodb://127.0.0.1:27017").connect();
const db = client.db(process.env.MONGODB_DB ?? "baremetal");
const suffix = randomBytes(5).toString("hex");
const sha = (s: string) => createHash("sha256").update(s).digest("hex");
const fakeIp = () => `203.0.113.${1 + Math.floor(Math.random() * 250)}`;
const users: ObjectId[] = [];

async function mkUser(label: string) {
  const _id = new ObjectId();
  const token = randomBytes(32).toString("hex");
  users.push(_id);
  await db.collection("users").insertOne({ _id, email: `rl-${label}-${suffix}@example.test`, name: "Rate limit test", passwordHash: "unused", emailVerified: true, experience: "some", role: "user", sessionVersion: 0, createdAt: new Date(Date.now() - 2 * 86400_000) });
  await db.collection("sessions").insertOne({ userId: _id, tokenHash: sha(token), sessionVersion: 0, createdAt: new Date(), lastSeenAt: new Date(), expiresAt: new Date(Date.now() + 3600_000) });
  return { id: _id, token };
}
const statuses = async (n: number, send: (i: number) => Promise<Response>) => {
  const out: number[] = [];
  for (let i = 0; i < n; i++) { const r = await send(i); await r.arrayBuffer(); out.push(r.status); }
  return out;
};

try {
  // ---- Export: 5 per hour per account (keyed on the account, so the address does not matter) ----
  const a = await mkUser("export-a");
  const b = await mkUser("export-b");
  const exportAs = (u: { token: string }, ip = fakeIp()) => fetch(`${base}/api/account/export`, { headers: { Cookie: `${SESSION_COOKIE}=${u.token}`, "X-Forwarded-For": ip } });
  const ipA = fakeIp();
  const ex = await statuses(7, () => exportAs(a, ipA));
  assert.deepEqual(ex.slice(0, 5), [200, 200, 200, 200, 200], `first five exports succeed: ${ex}`);
  assert.deepEqual(ex.slice(5), [429, 429], `the sixth and seventh are limited: ${ex}`);
  assert.equal((await exportAs(a, fakeIp())).status, 429, "changing the IP does not reset a per-account limit");
  assert.equal((await exportAs(b, ipA)).status, 200, "another account on the same IP is unaffected");
  console.log("export: 5 allowed, then 429 per account.");

  // ---- Shell create: 6 per 10 minutes per account, 20 per 10 minutes per IP. The limit is checked before the
  // shell service is contacted, so with no broker the allowed calls answer 503 and the rest 429. ----
  const post = (u: { token: string }, ip: string) => fetch(`${base}/api/shell`, {
    method: "POST", headers: { Origin: origin, "Content-Type": "application/json", Cookie: `${SESSION_COOKIE}=${u.token}`, "X-Forwarded-For": ip }, body: JSON.stringify({ cols: 80, rows: 24 }),
  });
  const s1 = await mkUser("shell-user");
  const sh = await statuses(8, () => post(s1, fakeIp()));
  const firstLimited = sh.indexOf(429);
  assert.notEqual(firstLimited, -1, `shell create must hit 429: ${sh}`);
  assert.ok(sh.slice(0, firstLimited).every((s) => s !== 429) && firstLimited >= 1, `calls before the limit are not 429: ${sh}`);
  assert.equal(firstLimited, 6, `exactly six creates are admitted per account: ${sh}`);
  assert.ok(sh.slice(6).every((s) => s === 429), `everything after is limited: ${sh}`);
  console.log(`shell create: per-account limit holds (${sh.join(" ")}).`);

  // Per-IP: many different accounts from one address.
  const sharedIp = fakeIp();
  const many = await Promise.all(Array.from({ length: 22 }, (_, i) => mkUser(`shell-ip-${i}`)));
  const ipStatuses = await statuses(22, (i) => post(many[i], sharedIp));
  assert.ok(ipStatuses.slice(0, 20).every((s) => s !== 429), `twenty creates per IP are admitted: ${ipStatuses}`);
  assert.deepEqual(ipStatuses.slice(20), [429, 429], `the 21st and 22nd from one IP are limited: ${ipStatuses}`);
  console.log("shell create: per-IP limit holds across accounts.");

  // ---- Analytics: 120 per minute per IP. Excess requests still answer 204 (no signal for the caller), so count stored events. ----
  const marker = `rl-${suffix}.example`;
  const consent = `${CONSENT_COOKIE}=${serializeConsent(true)}`;
  const track = (ip: string) => fetch(`${base}/api/analytics`, {
    method: "POST", headers: { Origin: origin, "Content-Type": "application/json", Cookie: consent, "X-Forwarded-For": ip, "User-Agent": "Mozilla/5.0 rate-limit-test" },
    body: JSON.stringify({ type: "pageview", path: "/", ref: `https://${marker}/` }),
  });
  const stored = () => db.collection("events").countDocuments({ ref: marker });
  const burstIp = fakeIp();
  const an = await statuses(130, () => track(burstIp));
  assert.ok(an.every((s) => s === 204), "analytics always answers 204");
  const n = await stored();
  assert.ok(n > 0, "analytics stored events (is consent/Origin accepted? pass the configured site origin as the second argument)");
  assert.equal(n, 120, `only 120 events per IP per minute are stored, got ${n}`);
  await track(fakeIp());
  assert.equal(await stored(), 121, "another IP is unaffected");
  console.log("analytics: 120 per minute per IP, excess dropped.");
  await db.collection("events").deleteMany({ ref: marker });
  console.log("Rate-limit checks passed.");
} finally {
  await db.collection("sessions").deleteMany({ userId: { $in: users } });
  await db.collection("users").deleteMany({ _id: { $in: users } });
  await db.collection("rate_limits").deleteMany({ key: { $regex: `^(export:(${users.map((u) => u.toHexString()).join("|")})|shell:create:user:(${users.map((u) => u.toHexString()).join("|")}))` } });
  await client.close();
}
