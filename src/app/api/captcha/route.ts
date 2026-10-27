import { NextResponse } from "next/server";
import { createChallenge } from "@/lib/captcha";
import { jsonError } from "@/lib/http";
import { clientIp, rateLimit } from "@/lib/rate-limit";

export async function GET(req: Request) {
  if (!(await rateLimit(`captcha:${clientIp(req.headers)}`, 30, 60))) return jsonError(429, "Too many requests.");
  return NextResponse.json(createChallenge(), { headers: { "Cache-Control": "no-store" } });
}
