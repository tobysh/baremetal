import { NextResponse } from "next/server";
import { getCurrentUser } from "@/lib/auth";
import { checkPractice, findPractice } from "@/lib/curriculum";
import { levelFor, practiceXpFor, titleBlurb } from "@/lib/gamification";
import { jsonError, readJson, sameOrigin } from "@/lib/http";
import { recordAttempt } from "@/lib/progress";
import { rateLimit } from "@/lib/rate-limit";
import { executeTest, RunnerBusyError, RunnerUnavailableError } from "@/lib/runner-client";

type Body = { exerciseId?: string; submission?: unknown };

/**
 * Tap-and-drag practice answers. The request is `{ exerciseId: "<lesson id>#practice", submission }` (see
 * PracticeSubmission) and the response is `{ ok, feedback?, explain?, outcome }`: `ok` means the answer is correct,
 * `outcome` is `{ saved, firstPass, xp, paced, reward }` with the same `reward` shape as /api/run. Answers are checked
 * here and never sent to the browser. A parsons/blanks answer that is not the canonical one is accepted only if the
 * assembled source passes the lesson's hidden tests.
 */
export async function POST(req: Request) {
  if (!sameOrigin(req)) return jsonError(403, "Cross-site request blocked.");
  const user = await getCurrentUser();
  if (!user) return jsonError(401, "Sign in to check practice answers.");
  if (!(await rateLimit(`practice:user:${user.id}`, 120, 10 * 60))) return jsonError(429, "You're answering very fast. Take a breath and try again in a few minutes.");

  const body = await readJson<Body>(req, 32 * 1024);
  if (!body || typeof body.exerciseId !== "string" || body.submission === undefined) return jsonError(400, "Invalid request.");
  const found = findPractice(body.exerciseId);
  if (!found) return jsonError(404, "Unknown practice exercise.");

  const check = checkPractice(found.practice, body.submission);
  if (!check.valid) return jsonError(400, check.error);

  try {
    let correct = check.exact;
    let feedback = check.feedback;
    if (!correct && check.testable && check.code) {
      // A different arrangement may still be a working program: the lesson's own hidden tests decide.
      const result = await executeTest(found.exercise, check.code, { keys: [`user:${user.id}`], signal: req.signal });
      correct = result.allPassed;
      if (!correct) feedback = result.compiled ? `${check.feedback ?? "Not quite."} The lesson's tests do not pass yet.` : `${check.feedback ?? "Not quite."} That arrangement does not compile.`;
    }

    const reward = await recordAttempt({
      userId: user.id,
      exerciseId: body.exerciseId,
      trackSlug: found.track.slug,
      code: JSON.stringify(body.submission).slice(0, 16 * 1024),
      passed: correct,
      difficulty: found.lesson.difficulty,
      practice: true,
    });
    const after = reward.totalXp != null ? levelFor(reward.totalXp) : null;
    const before = reward.totalXp != null ? levelFor(reward.totalXp - reward.xpAwarded) : null;
    return NextResponse.json({
      ok: correct,
      ...(correct ? { explain: found.practice.explain ?? null } : { feedback: feedback ?? "Not quite." }),
      outcome: {
        saved: true,
        firstPass: reward.firstPass,
        xp: reward.xpAwarded,
        xpWorth: practiceXpFor(found.lesson.difficulty),
        paced: reward.paced ?? false,
        reward: reward.firstPass && after ? { xp: reward.xpAwarded, totalXp: after.xp, level: after.level, title: after.title, blurb: titleBlurb(after.level), levelUp: after.level > (before?.level ?? 0) } : null,
      },
    });
  } catch (e) {
    if (req.signal.aborted) return new Response(null, { status: 499 });
    if (e instanceof RunnerUnavailableError) return jsonError(503, "The code runner is temporarily unavailable. Try again shortly.");
    if (e instanceof RunnerBusyError) return jsonError(503, "The code runner is busy right now. Please try again in a few seconds.");
    console.error("[practice] failure", e);
    return jsonError(500, "Something went wrong on our side. Please try again.");
  }
}
