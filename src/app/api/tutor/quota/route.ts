import { NextResponse } from "next/server";
import { getCurrentUser } from "@/lib/auth";
import { getDb } from "@/lib/db";
import { jsonError } from "@/lib/http";
import { rateLimit } from "@/lib/rate-limit";
import { HOUR, LIMITS } from "@/lib/tutor-limits";

/** Tutor questions left this hour, read without consuming any. Lets the chat show the allowance before the first question. */
export async function GET() {
  const user = await getCurrentUser();
  if (!user) return jsonError(401, "Sign in to chat with the tutor.");
  if (!(await rateLimit(`tutor:quota:${user.id}`, 30, 60))) return jsonError(429, "Too many requests.");
  const window = Math.floor(Date.now() / (HOUR * 1000));
  const doc = await (await getDb()).collection<{ key: string; count: number }>("rate_limits").findOne({ key: `tutor:user:h:${user.id}:${window}` });
  return NextResponse.json({ remaining: Math.max(0, LIMITS.userPerHour - (doc?.count ?? 0)), limit: LIMITS.userPerHour }, { headers: { "Cache-Control": "no-store" } });
}
