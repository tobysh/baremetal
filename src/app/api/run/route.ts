import { NextResponse } from "next/server";
import { getCurrentUser } from "@/lib/auth";
import { findExercise } from "@/lib/curriculum";
import { levelFor, xpFor } from "@/lib/gamification";
import { jsonError, readJson, sameOrigin } from "@/lib/http";
import { recordAttempt } from "@/lib/progress";
import { clientIp, rateLimit } from "@/lib/rate-limit";
import { runProgram, testExercise } from "@/lib/runner";

type Body = { exerciseId?: string; code?: string; stdin?: string; mode?: "run" | "test" };

const MAX_CODE = 64 * 1024;

export async function POST(req: Request) {
  if (!sameOrigin(req)) return jsonError(403, "Cross-site request blocked.");
  const body = await readJson<Body>(req);
  if (!body || typeof body.code !== "string" || typeof body.exerciseId !== "string") return jsonError(400, "Invalid request.");
  if (body.code.length > MAX_CODE) return jsonError(413, "Your code is over 64 KB — that's much more than any exercise needs.");
  const found = findExercise(body.exerciseId);
  if (!found) return jsonError(404, "Unknown exercise.");

  const user = await getCurrentUser();
  // Signed-in learners get a higher limit; anyone can try the editor.
  const key = user ? `run:user:${user.id}` : `run:ip:${clientIp(req.headers)}`;
  if (!(await rateLimit(key, user ? 120 : 30, 10 * 60))) {
    return jsonError(429, user ? "You're running code very fast — take a breath and try again in a few minutes." : "Run limit reached. Create a free account for more runs.");
  }

  try {
    if (body.mode === "test") {
      const result = await testExercise(found.exercise, body.code);
      const reward = user
        ? await recordAttempt(user.id, found.exercise.id, found.track.slug, body.code, result.allPassed, xpFor(found.lesson.difficulty))
        : null;
      const before = reward?.totalXp != null ? levelFor(reward.totalXp - reward.xpAwarded) : null;
      const after = reward?.totalXp != null ? levelFor(reward.totalXp) : null;
      return NextResponse.json({
        kind: "test",
        ...result,
        saved: !!user,
        reward: reward?.firstPass && after ? { xp: reward.xpAwarded, totalXp: after.xp, level: after.level, title: after.title, levelUp: after.level > (before?.level ?? 0) } : null,
      });
    }
    const stdin = typeof body.stdin === "string" ? body.stdin.slice(0, 16 * 1024) : "";
    const result = await runProgram(found.exercise.language, body.code, stdin);
    return NextResponse.json({ kind: "run", ...result });
  } catch (e) {
    console.error("[run] runner failure", e);
    return jsonError(500, "The code runner hit a problem on our side. Please try again.");
  }
}
