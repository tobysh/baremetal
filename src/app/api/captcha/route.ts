import { NextResponse } from "next/server";
import { createChallenge } from "@/lib/captcha";
import { hmac } from "@/lib/crypto";
import { jsonError } from "@/lib/http";
import { clientIp, ipLimit, rateLimit } from "@/lib/rate-limit";

export async function GET(req: Request) {
  const ip = clientIp(req.headers);
  if (!(await rateLimit(`captcha:${hmac("rate-ip", ip)}`, ipLimit(ip, 30), 60))) return jsonError(429, "Too many requests.");
  return NextResponse.json(createChallenge(), { headers: { "Cache-Control": "no-store" } });
}
