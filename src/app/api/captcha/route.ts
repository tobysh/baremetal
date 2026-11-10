import { NextResponse } from "next/server";
import { createChallenge, parseCaptchaForm } from "@/lib/captcha";
import { hmac } from "@/lib/crypto";
import { jsonError } from "@/lib/http";
import { envLimit } from "@/lib/security";
import { clientIp, ipLimit, rateLimit } from "@/lib/rate-limit";

export async function GET(req: Request) {
  const ip = clientIp(req.headers);
  if (!(await rateLimit(`captcha:${hmac("rate-ip", ip)}`, ipLimit(ip, envLimit("CAPTCHA_LIMIT_IP_MINUTE", 120)), 60))) return jsonError(429, "Too many requests.");
  const form = parseCaptchaForm(new URL(req.url).searchParams.get("form"));
  if (!form) return jsonError(400, "Unknown form.");
  return NextResponse.json(createChallenge(form, ip), { headers: { "Cache-Control": "no-store" } });
}
