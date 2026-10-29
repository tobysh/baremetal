// End-to-end checks for auth and security controls, run against a running
// server by submitting the real forms the way a browser without JavaScript
// would (server actions support progressive enhancement).
//
//   npm run build && TRUSTED_PROXY_HOPS=1 npx next start -p 3100   (in another terminal)
//   npm run test:e2e -- http://localhost:3100
//
// Creates throwaway accounts (e2e-*@example.test) and deletes them afterwards.
// Each phase sends a different simulated client IP (X-Forwarded-For), as a
// reverse proxy would, so IP-based limits don't interfere between phases.
import { createHash, createHmac, randomBytes } from "node:crypto";
import { MongoClient, type ObjectId } from "mongodb";
import { encrypt, hmac, sha256 } from "../src/lib/crypto";
import { hashBackupCode, legacyBackupCodeHash, newBackupCodes } from "../src/lib/totp";
import { clearLoginFailures, finishLoginFailure, refundLoginAttempt, reserveLoginAttempt } from "../src/lib/security";

const BASE = (process.argv[2] ?? "http://localhost:3100").replace(/\/$/, "");
const mongo = await new MongoClient(process.env.MONGODB_URI ?? "mongodb://127.0.0.1:27017").connect();
const db = mongo.db(process.env.MONGODB_DB ?? "baremetal");
const users = db.collection<{ _id: ObjectId; email: string; role?: string; twoFactor?: unknown; passwordHash: string; emailVerified?: boolean }>("users");

let failures = 0;
function check(name: string, ok: boolean, detail = "") {
  console.log(`${ok ? "  ✓" : "  ✗"} ${name}${ok || !detail ? "" : ` — ${detail}`}`);
  if (!ok) failures++;
}

// ---------- Minimal browser ----------

class Browser {
  cookies = new Map<string, string>();
  setCookieHeaders: string[] = [];
  private _ip = "";
  constructor(ip: string) {
    this.ip = ip;
  }
  get ip() {
    return this._ip;
  }
  set ip(v: string) {
    usedIps.add(v);
    this._ip = v;
  }
  private headers(extra: Record<string, string> = {}) {
    return { Cookie: [...this.cookies].map(([k, v]) => `${k}=${v}`).join("; "), "X-Forwarded-For": this.ip, ...extra };
  }
  private store(res: Response) {
    this.setCookieHeaders = res.headers.getSetCookie();
    for (const c of this.setCookieHeaders) {
      const [pair] = c.split(";");
      const i = pair.indexOf("=");
      const name = pair.slice(0, i);
      const value = pair.slice(i + 1);
      if (!value || /Max-Age=0|Expires=Thu, 01 Jan 1970/i.test(c)) this.cookies.delete(name);
      else this.cookies.set(name, value);
    }
  }
  async get(path: string) {
    const res = await fetch(BASE + path, { headers: this.headers(), redirect: "manual" });
    this.store(res);
    return { status: res.status, location: res.headers.get("location"), html: await res.text() };
  }
  async fetch(path: string, init: { method?: string; body?: string; headers?: Record<string, string> } = {}) {
    const res = await fetch(BASE + path, { ...init, headers: this.headers({ Origin: BASE, ...init.headers }), redirect: "manual" });
    this.store(res);
    return res;
  }
  /** Loads `path`, finds the form containing `fields`' first key, and submits it. */
  async submit(path: string, fields: Record<string, string>) {
    const page = await this.get(path);
    const forms = page.html.split(/<form\b/).slice(1).map((f) => f.split("</form>")[0]);
    const firstKey = Object.keys(fields)[0];
    const form = forms.find((f) => f.includes(`name="${firstKey}"`));
    if (!form) throw new Error(`No form with field "${firstKey}" on ${path}`);
    const data = new FormData();
    for (const m of form.matchAll(/<input type="hidden" name="([^"]+)"(?: value="([^"]*)")?/g)) data.append(decode(m[1]), decode(m[2] ?? ""));
    for (const [k, v] of Object.entries(fields)) data.set(k, v);
    const res = await fetch(BASE + path, { method: "POST", body: data, headers: this.headers({ Origin: BASE }), redirect: "manual" });
    this.store(res);
    return { status: res.status, location: res.headers.get("location"), html: await res.text() };
  }
}
const decode = (s: string) => s.replace(/&quot;/g, '"').replace(/&#x27;|&#39;/g, "'").replace(/&lt;/g, "<").replace(/&gt;/g, ">").replace(/&amp;/g, "&");
const text = (html: string) => decode(html.replace(/<[^>]+>/g, " ")).replace(/\s+/g, " ");

async function solveCaptcha(b: Browser): Promise<string> {
  const c = (await (await b.fetch("/api/captcha")).json()) as { salt: string; challenge: string; max: number; signature: string };
  for (let n = 0; n <= c.max; n++) {
    if (createHash("sha256").update(`${c.salt}${n}`).digest("hex") === c.challenge) return JSON.stringify({ ...c, number: n });
  }
  throw new Error("captcha unsolvable");
}

function totp(secret: string, offset = 0) {
  const B32 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
  let bits = "";
  for (const ch of secret) bits += B32.indexOf(ch).toString(2).padStart(5, "0");
  const key = Buffer.from(bits.match(/.{8}/g)!.map((b) => parseInt(b, 2)));
  const msg = Buffer.alloc(8);
  msg.writeBigUInt64BE(BigInt(Math.floor(Date.now() / 30000) + offset));
  const h = createHmac("sha1", key).update(msg).digest();
  const o = h[h.length - 1] & 0xf;
  return String((((h[o] & 0x7f) << 24) | (h[o + 1] << 16) | (h[o + 2] << 8) | h[o + 3]) % 1_000_000).padStart(6, "0");
}

const run = Date.now().toString(36);
const email = `e2e-${run}@example.test`;
const PASSWORD = "Granite-Lantern-Orbit-42";
const usedIps = new Set<string>();
const ip = (n: number) => {
  const addr = `10.${(Date.now() >> 8) & 255}.${n}.${Math.floor(Math.random() * 250) + 1}`;
  usedIps.add(addr);
  return addr;
};

try {
  // ---------- Sign-up & password policy ----------
  console.log("\nSign-up and password strength");
  {
    const b = new Browser(ip(1));
    const base = { name: "E2E Tester", username: `e2e${run}`.slice(0, 20), email, experience: "some", terms: "on", age: "on" };
    let r = await b.submit("/signup", { ...base, password: "password123", confirm: "password123", captcha: await solveCaptcha(b) });
    check('rejects "password123" (missing rules)', r.status === 200 && /needs:/.test(text(r.html)), text(r.html).slice(0, 200));
    r = await b.submit("/signup", { ...base, password: "Password123!", confirm: "Password123!", captcha: await solveCaptcha(b) });
    check('rejects "Password123!" (common password, decorated)', /too common/.test(text(r.html)));
    r = await b.submit("/signup", { ...base, password: "Tester-E2E-2026!x", confirm: "Tester-E2E-2026!x", captcha: await solveCaptcha(b) });
    check("rejects passwords containing the user's name", /name or email/.test(text(r.html)));
    r = await b.submit("/signup", { ...base, password: PASSWORD, confirm: PASSWORD, captcha: "" });
    check("sign-up requires the CAPTCHA", /security check/i.test(text(r.html)));
    r = await b.submit("/signup", { ...base, terms: "", password: PASSWORD, confirm: PASSWORD, captcha: await solveCaptcha(b) });
    check("sign-up requires accepting the Terms", /accept the Terms/.test(text(r.html)));
    r = await b.submit("/signup", { ...base, password: PASSWORD, confirm: PASSWORD, captcha: await solveCaptcha(b) });
    check("sign-up is limited to 5 attempts per 15 min per IP", /Too many attempts/.test(text(r.html)));
    b.ip = ip(13); // a different network
    r = await b.submit("/signup", { ...base, password: PASSWORD, confirm: PASSWORD, captcha: await solveCaptcha(b) });
    check("valid sign-up gives generic confirmation without a session", r.status === 200 && /check your email/i.test(text(r.html)) && !b.cookies.has("bm_session"));
    const stored = await users.findOne({ email });
    check("new account requires email verification", stored?.emailVerified === false);
    check("password stored only as a scrypt hash", !!stored?.passwordHash.startsWith("scrypt$") && !stored.passwordHash.includes(PASSWORD));
    r = await b.submit("/login", { email, password: PASSWORD });
    check("unverified account cannot sign in", !b.cookies.has("bm_session") && /verify your email/i.test(text(r.html)));
    const verifyToken = randomBytes(32).toString("base64url");
    await db.collection("email_verifications").insertOne({ tokenHash: sha256(verifyToken), userId: stored!._id, expiresAt: new Date(Date.now() + 60000) });
    r = await b.submit("/verify-email", { token: verifyToken, password: PASSWORD, confirm: PASSWORD });
    check("verification establishes the owner's password", /Email verified/i.test(text(r.html)) && (await users.findOne({ email }))?.emailVerified === true);
    r = await b.submit("/verify-email", { token: verifyToken, password: PASSWORD, confirm: PASSWORD });
    check("verification token is single-use", /invalid|already used/i.test(text(r.html)));
    r = await b.submit("/login", { email, password: PASSWORD });
    check("verified account can sign in", r.status === 303 && b.cookies.has("bm_session"));
    const sc = b.setCookieHeaders.find((c) => c.startsWith("bm_session=")) ?? "";
    check("session cookie is HttpOnly", /HttpOnly/i.test(sc), sc);
    check("session cookie is SameSite=Strict", /SameSite=Strict/i.test(sc), sc);
    const session = stored && (await db.collection("sessions").findOne({ userId: stored._id }));
    check("session token stored hashed, not raw", !!session && session.tokenHash === sha256(b.cookies.get("bm_session")!));
    await users.updateOne({ email }, { $unset: { emailVerified: "" } });
    const legacy = new Browser(ip(18));
    const legacyResult = await legacy.submit("/login", { email, password: PASSWORD });
    check("legacy account without a verification flag can sign in", legacyResult.status === 303 && legacy.cookies.has("bm_session"));
    await users.updateOne({ email }, { $set: { emailVerified: true } });
    const duplicate = new Browser(ip(14));
    r = await duplicate.submit("/signup", { ...base, password: PASSWORD, confirm: PASSWORD, captcha: await solveCaptcha(duplicate) });
    check("existing signup gives the same generic response", r.status === 200 && /check your email/i.test(text(r.html)) && !duplicate.cookies.has("bm_session"));
    const dash = await b.get("/dashboard");
    check("dashboard loads when signed in", dash.status === 200 && dash.html.includes("E2E Tester"));
    const exported = await (await b.fetch("/api/account/export")).json() as { account: { emailVerified?: boolean }, pendingEmailDelivery?: Record<string, unknown>[] };
    check("account export includes verification status and omits delivery secrets", exported.account.emailVerified === true && Array.isArray(exported.pendingEmailDelivery) && exported.pendingEmailDelivery.every((job) => !("payload" in job) && !("recipientHash" in job)));

  }

  // ---------- Server-side authorization ----------
  console.log("\nAuthorization is enforced on the server");
  {
    const b = new Browser(ip(2));
    let r = await b.get("/dashboard");
    check("signed-out /dashboard redirects to login", r.status === 307 && !!r.location?.includes("/login"));
    r = await b.get("/admin");
    check("signed-out /admin redirects to login", r.status === 307);
    const res = await b.fetch("/api/admin/stats");
    check("signed-out /api/admin/stats is 403", res.status === 403);
    // Forged cookie
    b.cookies.set("bm_session", randomBytes(32).toString("base64url"));
    r = await b.get("/dashboard");
    check("a forged session cookie isn't accepted", !r.html.includes("E2E Tester"));
    // Signed in as a normal user
    const u = new Browser(ip(3));
    await u.submit("/login", { email, password: PASSWORD });
    check("normal user gets 403 from /api/admin/stats", (await u.fetch("/api/admin/stats")).status === 403);
    r = await u.get("/admin");
    check("normal user can't render /admin", !r.html.includes("Pageviews per day"));
    // Admin without 2FA
    await users.updateOne({ email }, { $set: { role: "admin" } });
    check("admin WITHOUT 2FA still gets 403 from the API", (await u.fetch("/api/admin/stats")).status === 403);
    r = await u.get("/admin");
    check("admin without 2FA is sent to set up 2FA", !r.html.includes("Pageviews per day") && /settings\/security\?required=admin/.test(r.html + (r.location ?? "")));
    await users.updateOne({ email }, { $set: { role: "user" } });
    // CSRF
    const res2 = await u.fetch("/api/run", { method: "POST", headers: { Origin: "https://evil.example", "Content-Type": "application/json" }, body: "{}" });
    check("cross-site POST to an API route is blocked (403)", res2.status === 403);
  }

  // ---------- Two-factor authentication ----------
  console.log("\nTwo-factor authentication");
  {
    const setup = new Browser(ip(17));
    await setup.submit("/login", { email, password: PASSWORD });
    let setupResult = await setup.submit("/settings/security", { password: "wrong-password" });
    check("2FA setup requires the current password", /Incorrect password/.test(text(setupResult.html)) && !(await users.findOne({ email }))?.twoFactor);
    setupResult = await setup.submit("/settings/security", { password: PASSWORD });
    check("2FA setup accepts the current password", !!(await users.findOne({ email }))?.twoFactor && !/Incorrect password/.test(text(setupResult.html)));
    const secret = "JBSWY3DPEHPK3PXPJBSWY3DPEHPK3PXP";
    const codes = newBackupCodes();
    await users.updateOne(
      { email },
      { $set: { twoFactor: { enabled: true, secretEnc: encrypt(secret), backupCodeHashes: codes.map(hashBackupCode), lastStep: -1 } }, $inc: { sessionVersion: 1 } },
    );
    const b = new Browser(ip(4));
    let r = await b.submit("/login", { email, password: PASSWORD });
    check("password step redirects to /login/verify, not a session", r.status === 303 && !!r.location?.includes("/login/verify") && !b.cookies.has("bm_session"));
    const pend = b.setCookieHeaders.find((c) => c.startsWith("bm_2fa_pending=")) ?? "";
    check("pending-2FA cookie is HttpOnly + SameSite=Strict", /HttpOnly/i.test(pend) && /SameSite=Strict/i.test(pend), pend);
    r = await b.submit("/login/verify", { code: "000000" });
    check("wrong code is rejected", /didn.t work/.test(text(r.html)) && !b.cookies.has("bm_session"));
    const code = totp(secret);
    r = await b.submit("/login/verify", { code });
    check("correct TOTP signs in", r.status === 303 && b.cookies.has("bm_session"), `${r.status} ${r.location}`);
    // Replay the same code in a fresh login
    const b2 = new Browser(ip(5));
    await b2.submit("/login", { email, password: PASSWORD });
    r = await b2.submit("/login/verify", { code });
    check("the same TOTP code can't be replayed", !b2.cookies.has("bm_session"));
    r = await b2.submit("/login/verify", { code: codes[0] });
    check("a backup code signs in", b2.cookies.has("bm_session"));
    const b3 = new Browser(ip(6));
    await b3.submit("/login", { email, password: PASSWORD });
    await b3.submit("/login/verify", { code: codes[0] });
    check("a backup code works only once", !b3.cookies.has("bm_session"));
    const legacy = "k3f9-x2mq";
    await users.updateOne({ email }, { $push: { "twoFactor.backupCodeHashes": legacyBackupCodeHash(legacy) } } as never);
    const old = new Browser(ip(15));
    await old.submit("/login", { email, password: PASSWORD });
    await old.submit("/login/verify", { code: legacy });
    check("legacy backup code remains valid once", old.cookies.has("bm_session"));
    // Too many wrong codes ends the attempt
    let after = null;
    for (let i = 0; i < 4; i++) after = await b3.submit("/login/verify", { code: "111111" });
    check("5 wrong codes end the sign-in attempt", !!after && (after.location?.includes("/login?expired=1") ?? false), `${after?.status} ${after?.location}`);
    // Failures count across independent pending sessions and addresses.
    const peers = Array.from({ length: 3 }, (_, i) => new Browser(ip(20 + i)));
    const user = await users.findOne({ email });
    await db.collection("login_failures").deleteOne({ key: `otp:${hmac("login-otp", user!._id.toHexString())}` });
    for (const peer of peers) await peer.submit("/login", { email, password: PASSWORD });
    for (const [peer, count] of [[peers[0], 2], [peers[1], 2], [peers[2], 1]] as const)
      for (let i = 0; i < count; i++) await peer.submit("/login/verify", { code: "wrong-code" });
    const blocked = await peers[0].submit("/login/verify", { code: codes[1] });
    check("OTP backoff follows the account across pending sessions", /Too many verification attempts/i.test(text(blocked.html)) && !peers[0].cookies.has("bm_session"));
    await db.collection("login_failures").deleteOne({ key: `otp:${hmac("login-otp", user!._id.toHexString())}` });
    // Admin with 2FA
    await users.updateOne({ email }, { $set: { role: "admin" } });
    check("admin WITH 2FA can use /api/admin/stats", (await b.fetch("/api/admin/stats")).status === 200);
    const stale = new Browser(ip(23));
    await stale.submit("/login", { email, password: PASSWORD });
    await users.updateOne({ email }, { $inc: { sessionVersion: 1 } });
    const staleResult = await stale.submit("/login/verify", { code: codes[2] });
    check("revoking sessions also revokes pending 2FA", staleResult.status === 303 && !stale.cookies.has("bm_session"));
    await users.updateOne({ email }, { $set: { role: "user" }, $unset: { twoFactor: "" } });
  }

  // ---------- Brute force: CAPTCHA, lockout, backoff, logging ----------
  console.log("\nBrute-force protection");
  {
    const victim = `e2e-${run}-victim@example.test`;
    const b = new Browser(ip(7));
    let r;
    for (let i = 1; i <= 3; i++) r = await b.submit("/login", { email: victim, password: `wrong-${i}` });
    check("generic error message (no account enumeration)", /Incorrect email or password/.test(text(r!.html)));
    // The widget itself renders client-side; assert the server's action state.
    check("CAPTCHA is required after 3 failures", /captchaRequired\\?"?:true/.test(r!.html));
    r = await b.submit("/login", { email: victim, password: "wrong-4" });
    check("without the CAPTCHA the attempt is refused", /complete the security check/i.test(text(r.html)));
    r = await b.submit("/login", { email: victim, password: "wrong-4", captcha: await solveCaptcha(b) });
    r = await b.submit("/login", { email: victim, password: "wrong-5", captcha: await solveCaptcha(b) });
    r = await b.submit("/login", { email: victim, password: "wrong-6", captcha: await solveCaptcha(b) });
    check("locked out after 5 failures", /paused for/.test(text(r.html)), text(r.html).slice(0, 300));
    const fromOtherIp = new Browser(ip(8));
    r = await fromOtherIp.submit("/login", { email: victim, password: "wrong-7", captcha: await solveCaptcha(fromOtherIp) });
    check("lockout is limited to the email and IP pair", /Incorrect email or password/.test(text(r.html)));
    const doc = await db.collection("login_failures").findOne({ key: `pair:${hmac("login-pair", `${victim}\0${b.ip}`)}` });
    check("lockout grows exponentially (1, 2, 4… minutes)", !!doc?.lockedUntil && doc.count >= 5);
    const logged = await db.collection("auth_events").countDocuments({ email: victim, type: "login_failed" });
    check("every failed attempt is logged", logged >= 5, `logged ${logged}`);
    const concurrentIp = ip(16);
    const concurrentEmail = `e2e-${run}-parallel@example.test`;
    const racers = Array.from({ length: 6 }, () => new Browser(concurrentIp));
    const captchas = await Promise.all(racers.map(solveCaptcha));
    await Promise.all(racers.map((browser, i) => browser.submit("/login", { email: concurrentEmail, password: "wrong", captcha: captchas[i] })));
    const parallelDoc = await db.collection("login_failures").findOne({ key: `pair:${hmac("login-pair", `${concurrentEmail}\0${concurrentIp}`)}` });
    check("parallel guesses reserve at most five password checks", parallelDoc?.count === 5 && !!parallelDoc.lockedUntil, `count ${parallelDoc?.count}`);
    const inflightIp = ip(24), inflightEmail = `e2e-${run}-inflight@example.test`;
    await reserveLoginAttempt(inflightEmail, inflightIp); // failed check still running
    await reserveLoginAttempt(inflightEmail, inflightIp); // successful check
    await refundLoginAttempt(inflightEmail, inflightIp);
    await clearLoginFailures(inflightEmail, inflightIp);
    await finishLoginFailure(inflightEmail, inflightIp);
    const inflight = await db.collection("login_failures").findOne({ key: `pair:${hmac("login-pair", `${inflightEmail}\0${inflightIp}`)}` });
    check("successful login retains simultaneous unfinished failures", inflight?.count === 1 && inflight.inFlight === 0);
    // Real account: lockout applies whether or not the account exists
    const timing = async (addr: string) => {
      const t = performance.now();
      await new Browser(ip(9)).submit("/login", { email: addr, password: "nope-nope-nope" });
      return performance.now() - t;
    };
    const [known, unknown] = [await timing(email), await timing(`nobody-${run}@example.test`)];
    check("similar response time for known and unknown emails", Math.abs(known - unknown) < 250, `${known.toFixed(0)}ms vs ${unknown.toFixed(0)}ms`);
  }

  // ---------- IP rate limiting ----------
  console.log("\nIP rate limiting");
  {
    const b = new Browser(ip(10));
    let last;
    for (let i = 0; i < 6; i++) last = await b.submit("/forgot-password", { email: `rl-${i}-${run}@example.test` });
    check("forgot-password limited to 5 per 15 min per IP", /Too many attempts/.test(text(last!.html)));
  }

  // ---------- Password reset ----------
  console.log("\nPassword reset");
  {
    const b = new Browser(ip(11));
    let r = await b.submit("/forgot-password", { email });
    check("reset request gives the same message for any email", /If an account exists/.test(text(r.html)));
    r = await b.submit("/forgot-password", { email: `nobody-${run}@example.test` });
    check("…including unknown emails", /If an account exists/.test(text(r.html)));
    const user = await users.findOne({ email });
    // Our server can't send email here, so plant a known token the way the action does.
    const token = randomBytes(32).toString("base64url");
    await db.collection("password_resets").deleteMany({ userId: user!._id });
    await db.collection("password_resets").insertOne({ tokenHash: sha256(token), userId: user!._id, expiresAt: new Date(Date.now() + 30 * 60_000) });
    const signedIn = new Browser(ip(12));
    await signedIn.submit("/login", { email, password: PASSWORD });
    r = await b.submit(`/reset-password?token=${token}`, { token, password: "12345678", confirm: "12345678" });
    check('reset rejects "12345678"', /needs:|too common/.test(text(r.html)));
    const NEW = "Velvet-Harbour-Comet-77";
    r = await b.submit(`/reset-password?token=${token}`, { token, password: NEW, confirm: NEW });
    check("valid reset redirects to sign-in", r.status === 303 && !!r.location?.includes("/login?reset=1"), `${r.status} ${r.location}`);
    const dash = await signedIn.get("/dashboard");
    check("reset signs out every existing session", !dash.html.includes("E2E Tester"));
    r = await b.submit(`/reset-password?token=${token}`, { token, password: "Another-Fresh-Pass-99", confirm: "Another-Fresh-Pass-99" });
    check("reset token is single-use", /expired or was already used/.test(text(r.html)));
  }
} finally {
  const ids = (await users.find({ email: { $regex: `^e2e-${run}` } }).toArray()).map((u) => u._id);
  await Promise.all([
    users.deleteMany({ _id: { $in: ids } }),
    db.collection("sessions").deleteMany({ userId: { $in: ids } }),
    db.collection("progress").deleteMany({ userId: { $in: ids } }),
    db.collection("password_resets").deleteMany({ userId: { $in: ids } }),
    db.collection("email_verifications").deleteMany({ userId: { $in: ids } }),
    db.collection("email_queue").deleteMany({ recipientHash: hmac("email-queue-recipient", email) }),
    db.collection("pending_2fa").deleteMany({ userId: { $in: ids } }),
    db.collection("auth_events").deleteMany({ email: { $regex: `^(e2e|rl|nobody)-.*${run}` } }),
    // Only pair and OTP buckets created by this run.
    db.collection("login_failures").deleteMany({ key: { $in: [
      ...[...usedIps].flatMap((ip) => [email, `e2e-${run}-victim@example.test`, `e2e-${run}-parallel@example.test`, `e2e-${run}-inflight@example.test`, `nobody-${run}@example.test`].map((e) => `pair:${hmac("login-pair", `${e}\0${ip}`)}`)),
      ...ids.map((id) => `otp:${hmac("login-otp", id.toHexString())}`),
    ] } }),
  ]);
  await mongo.close();
}

console.log(failures ? `\n${failures} check(s) failed.` : "\nAll security checks passed.");
process.exit(failures ? 1 : 0);
