import { NextResponse } from "next/server";
import { getCurrentUser } from "@/lib/auth";
import { findExercise } from "@/lib/curriculum";
import { jsonError, readJson, sameOrigin } from "@/lib/http";
import { saveCode } from "@/lib/progress";
import { rateLimit } from "@/lib/rate-limit";

/** Autosave of the editor contents for signed-in learners. */
export async function PUT(req: Request) {
  if (!sameOrigin(req)) return jsonError(403, "Cross-site request blocked.");
  const user = await getCurrentUser();
  if (!user) return jsonError(401, "Sign in to save your progress.");
  const body = await readJson<{ exerciseId?: string; code?: string }>(req);
  if (!body || typeof body.code !== "string" || typeof body.exerciseId !== "string" || body.code.length > 64 * 1024)
    return jsonError(400, "Invalid request.");
  const found = findExercise(body.exerciseId);
  if (!found) return jsonError(404, "Unknown exercise.");
  if (!(await rateLimit(`autosave:${user.id}`, 120, 60))) return jsonError(429, "Saving too often.");
  await saveCode(user.id, found.exercise.id, found.track.slug, body.code);
  return NextResponse.json({ ok: true });
}
