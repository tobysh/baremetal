// End-to-end checks for auth and security controls, run against a running
// server by submitting the real forms the way a browser without JavaScript
// would (server actions support progressive enhancement).
//
//   npm run build && npx next start -p 3100   (in another terminal)
//   npm run test:e2e -- http://localhost:3100
//
// Creates throwaway accounts (e2e-*@example.test) and deletes them afterwards.
// Each phase sends a different simulated client IP (X-Forwarded-For), as a
// reverse proxy would, so IP-based limits don't interfere between phases.
import { createHash, createHmac, randomBytes } from "node:crypto";
import { MongoClient, type ObjectId } from "mongodb";
import { encrypt, sha256 } from "../src/lib/crypto";
import { hashBackupCode, newBackupCodes } from "../src/lib/totp";

const BASE = (process.argv[2] ?? "http://localhost:3100").replace(/\/$/, "");
const mongo = await new MongoClient(process.env.MONGODB_URI ?? "mongodb://127.0.0.1:27017").connect();
const db = mongo.db(process.env.MONGODB_DB ?? "baremetal");
const users = db.collection<{ _id: ObjectId; email: string; role?: string; twoFactor?: unknown; passwordHash: string }>("users");

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
    check("valid sign-up redirects to /learn", r.status === 303 && !!r.location?.includes("/learn"), `${r.status} ${r.location}`);
    const sc = b.setCookieHeaders.find((c) => c.startsWith("bm_session=")) ?? "";
    check("session cookie is HttpOnly", /HttpOnly/i.test(sc), sc);
    check("session cookie is SameSite=Strict", /SameSite=Strict/i.test(sc), sc);
    const stored = await users.findOne({ email });
    check("password stored only as a scrypt hash", !!stored?.passwordHash.startsWith("scrypt$") && !stored.passwordHash.includes(PASSWORD));
    const session = stored && (await db.collection("sessions").findOne({ userId: stored._id }));
    check("session token stored hashed, not raw", !!session && session.tokenHash === sha256(b.cookies.get("bm_session")!));
    const dash = await b.get("/dashboard");
    check("dashboard loads when signed in", dash.status === 200 && dash.html.includes("E2E Tester"));
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
    // Too many wrong codes ends the attempt
    for (let i = 0; i < 4; i++) await b3.submit("/login/verify", { code: "111111" });
    r = await b3.get("/login/verify");
    const after = await b3.submit("/login/verify", { code: "222222" }).catch(() => null);
    check("5 wrong codes end the sign-in attempt", !!after && (after.location?.includes("/login?expired=1") ?? false), `${after?.status} ${after?.location}`);
    // Admin with 2FA
    await users.updateOne({ email }, { $set: { role: "admin" } });
    check("admin WITH 2FA can use /api/admin/stats", (await b.fetch("/api/admin/stats")).status === 200);
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
    check("lockout follows the account across IPs", /paused for/.test(text(r.html)));
    const doc = await db.collection("login_failures").findOne({ key: `email:${sha256(victim)}` });
    check("lockout grows exponentially (1, 2, 4… minutes)", !!doc?.lockedUntil && doc.count >= 5);
    const logged = await db.collection("auth_events").countDocuments({ email: victim, type: "login_failed" });
    check("every failed attempt is logged", logged >= 5, `logged ${logged}`);
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
    db.collection("pending_2fa").deleteMany({ userId: { $in: ids } }),
    db.collection("auth_events").deleteMany({ email: { $regex: `^(e2e|rl|nobody)-.*${run}` } }),
    // Only the lockout records this run created.
    db.collection("login_failures").deleteMany({
      $or: [{ key: { $in: [...usedIps].map((a) => `ip:${a}`) } }, { key: { $in: [email, `e2e-${run}-victim@example.test`, `nobody-${run}@example.test`].map((e) => `email:${sha256(e)}`) } }],
    }),
  ]);
  await mongo.close();
}

console.log(failures ? `\n${failures} check(s) failed.` : "\nAll security checks passed.");
process.exit(failures ? 1 : 0);
