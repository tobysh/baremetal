import { createHash } from "node:crypto";
import { cookies } from "next/headers";
import { getDb } from "@/lib/db";
import { hmac } from "@/lib/crypto";
import { CONSENT_COOKIE, parseConsent } from "@/lib/consent";
import { readJson, sameOrigin } from "@/lib/http";
import { clientIp, ipLimit, rateLimit } from "@/lib/rate-limit";

type Body = { type?: string; path?: string; ref?: string; name?: string };
const ALLOWED_EVENTS = new Set(["pageview", "signup_cta", "run", "test_passed", "tutor_open", "hint_open"]);

/**
 * First-party, consent-gated analytics. Stores no IP and no cookie: the visitor
 * id is a hash of IP + user agent + a secret that rotates daily, so the same
 * person can't be followed across days.
 */
export async function POST(req: Request) {
  const empty = new Response(null, { status: 204 });
  if (!sameOrigin(req)) return empty;
  const consent = parseConsent((await cookies()).get(CONSENT_COOKIE)?.value);
  if (!consent?.analytics) return empty;

  const ip = clientIp(req.headers);
  if (!(await rateLimit(`analytics:${hmac("rate-ip", ip)}`, ipLimit(ip, 120), 60))) return empty;

  const body = await readJson<Body>(req, 4096);
  if (!body?.type || !ALLOWED_EVENTS.has(body.type)) return empty;

  const ua = req.headers.get("user-agent") ?? "";
  if (/bot|crawler|spider|headless|preview/i.test(ua)) return empty;
  const day = new Date().toISOString().slice(0, 10);
  const visitor = createHash("sha256").update(`${hmac("analytics-salt", day)}|${ip}|${ua}`).digest("hex").slice(0, 16);

  let refHost: string | null = null;
  try {
    refHost = body.ref ? new URL(body.ref).host : null;
  } catch {}
  const path = typeof body.path === "string" ? body.path.split("?")[0].slice(0, 200) : "/";

  const db = await getDb();
  await db.collection("events").insertOne({
    type: body.type,
    path,
    ref: refHost && refHost !== req.headers.get("host") ? refHost : null,
    device: /mobi|android|iphone|ipad/i.test(ua) ? "mobile" : "desktop",
    visitor,
    ts: new Date(),
  });
  return empty;
}
