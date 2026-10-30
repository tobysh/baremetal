import { NextResponse } from "next/server";
import { getCurrentUser } from "@/lib/auth";
import { findExercise } from "@/lib/curriculum";
import { jsonError, readJson, sameOrigin } from "@/lib/http";
import { recordHint } from "@/lib/progress";
import { clientIp, ipLimit, rateLimit } from "@/lib/rate-limit";
import { hmac } from "@/lib/crypto";

/** Record access before disclosing authored hints, including requests outside the UI. */
export async function POST(req: Request) {
  if (!sameOrigin(req)) return jsonError(403, "Cross-site request blocked.");
  const user = await getCurrentUser();
  const ip = clientIp(req.headers);
  if (!(await rateLimit(user ? `hint:user:${user.id}` : `hint:ip:${hmac("rate-ip", ip)}`, user ? 120 : ipLimit(ip, 60), 60))) return jsonError(429, "Too many requests.");

  const body = await readJson<{ exerciseId?: string; index?: number }>(req, 4096);
  if (!body || typeof body.exerciseId !== "string" || !Number.isInteger(body.index)) return jsonError(400, "Invalid request.");
  const found = findExercise(body.exerciseId);
  if (!found || found.isChallenge) return jsonError(404, "Unknown exercise.");
  const index = body.index!;
  if (index < 0 || index >= found.exercise.hints.length) return jsonError(400, "Unknown hint.");
  if (user) await recordHint(user.id, found.exercise.id, found.track.slug, index);
  return NextResponse.json({ hints: found.exercise.hints.slice(0, index + 1) }, { headers: { "Cache-Control": "no-store" } });
}
