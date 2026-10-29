import { NextResponse } from "next/server";
import { getCurrentUser } from "@/lib/auth";
import { findExercise } from "@/lib/curriculum";
import { levelFor, titleBlurb } from "@/lib/gamification";
import { jsonError, readJson, sameOrigin } from "@/lib/http";
import { getProgress, recordAttempt, recordReviewPass } from "@/lib/progress";
import { evaluateAchievements, trackCompleteFor, type TrackComplete, type Unlocked } from "@/lib/rewards";
import { clientIp, ipLimit, rateLimit } from "@/lib/rate-limit";
import { runProgram, testExercise, RunnerBusyError, withRunnerRequest } from "@/lib/runner";

type Body = { exerciseId?: string; code?: string; stdin?: string; mode?: "run" | "test"; review?: boolean };

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
  const ip = clientIp(req.headers);
  const key = user ? `run:user:${user.id}` : `run:ip:${ip}`;
  if (!(await rateLimit(key, user ? 120 : ipLimit(ip, 30), 10 * 60))) {
    return jsonError(429, user ? "You're running code very fast — take a breath and try again in a few minutes." : "Run limit reached. Create a free account for more runs.");
  }

  // A challenge opens once its lesson is passed.
  if (found.isChallenge && body.mode === "test") {
    if (!user) return jsonError(401, "Sign in to attempt challenges.");
    if ((await getProgress(user.id, found.lesson.exercise.id))?.status !== "passed") return jsonError(403, "Pass the lesson first; its challenge unlocks afterwards.");
  }

  try {
    if (body.mode === "test") {
      const result = await withRunnerRequest([`ip:${ip}`, ...(user ? [`user:${user.id}`] : [])], req.signal, () => testExercise(found.exercise, body.code!));
      const isReview = !!body.review && !found.isChallenge;
      let achievements: Unlocked[] = [];
      let trackComplete: TrackComplete | null = null;
      let reviewed = false;
      let reward: Awaited<ReturnType<typeof recordAttempt>> | null = null;
      if (user && isReview) {
        // Reviews re-solve a passed lesson from a fresh starter. They move the schedule, never XP or attempts.
        if (result.allPassed) reviewed = await recordReviewPass(user.id, found.exercise.id);
        if (reviewed) achievements = (await evaluateAchievements(user.id)).unlocked;
      } else if (user) {
        reward = await recordAttempt({
          userId: user.id,
          exerciseId: found.exercise.id,
          trackSlug: found.track.slug,
          code: body.code,
          passed: result.allPassed,
          difficulty: found.lesson.difficulty,
          challenge: found.isChallenge,
        });
        if (reward.firstPass) {
          const evaluated = await evaluateAchievements(user.id);
          achievements = evaluated.unlocked;
          trackComplete = found.isChallenge ? null : trackCompleteFor(found.track.slug, evaluated.ctx);
        }
      }
      const before = reward?.totalXp != null ? levelFor(reward.totalXp - reward.xpAwarded) : null;
      const after = reward?.totalXp != null ? levelFor(reward.totalXp) : null;
      return NextResponse.json({
        kind: "test",
        ...result,
        saved: !!user,
        paced: reward?.paced ?? false,
        unaided: reward?.unaided ?? false,
        reviewed,
        achievements,
        trackComplete,
        reward: reward?.firstPass && after ? { xp: reward.xpAwarded, totalXp: after.xp, level: after.level, title: after.title, blurb: titleBlurb(after.level), levelUp: after.level > (before?.level ?? 0) } : null,
      });
    }
    const stdin = typeof body.stdin === "string" ? body.stdin.slice(0, 16 * 1024) : "";
    const result = await withRunnerRequest([`ip:${ip}`, ...(user ? [`user:${user.id}`] : [])], req.signal, () => runProgram(found.exercise.language, body.code!, stdin));
    return NextResponse.json({ kind: "run", ...result });
  } catch (e) {
    if (req.signal.aborted) return new Response(null, { status: 499 });
    if (e instanceof RunnerBusyError) return jsonError(503, "The code runner is busy right now. Please try again in a few seconds.");
    console.error("[run] runner failure", e);
    return jsonError(500, "The code runner hit a problem on our side. Please try again.");
  }
}
