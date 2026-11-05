import { NextResponse } from "next/server";
import { getCurrentUser } from "@/lib/auth";
import { checkPractice, findExercise, revealAnswer } from "@/lib/curriculum";
import { jsonError, readJson, sameOrigin } from "@/lib/http";
import { rateLimit } from "@/lib/rate-limit";
import { isReviewDue, reviewDeck, settleReviewCard } from "@/lib/review-cards";

/**
 * Graded review cards: `{ exerciseId, q, submission }` answers question `q` of that lesson's deck. The answer is
 * checked here and only revealed once the schedule has moved, so a due card can be answered once: right moves the
 * lesson to its next interval, wrong starts it over. No XP, no Codex credit. Only exact answers count.
 */
export async function POST(req: Request) {
  if (!sameOrigin(req)) return jsonError(403, "Cross-site request blocked.");
  const user = await getCurrentUser();
  if (!user) return jsonError(401, "Sign in to save your reviews.");
  const body = await readJson<{ exerciseId?: string; q?: number; submission?: unknown }>(req, 16 * 1024);
  if (!body || typeof body.exerciseId !== "string" || !Number.isInteger(body.q) || body.submission === undefined) return jsonError(400, "Invalid request.");
  const found = findExercise(body.exerciseId);
  if (!found || found.isChallenge) return jsonError(404, "Unknown exercise.");
  const question = reviewDeck(found.lesson)[body.q!];
  if (!question) return jsonError(404, "Unknown question.");
  if (!(await rateLimit(`review-card:${user.id}`, 120, 60))) return jsonError(429, "Too many requests.");
  if (!(await isReviewDue(user.id, found.exercise.id))) return jsonError(409, "This review is not due, or you already answered it.");

  const check = checkPractice(question, body.submission);
  if (!check.valid) return jsonError(400, check.error);
  const moved = await settleReviewCard(user.id, found.exercise.id, check.exact);
  if (!moved) return jsonError(409, "This review is not due, or you already answered it.");
  return NextResponse.json({
    ok: true,
    correct: check.exact,
    feedback: check.exact ? null : check.feedback ?? null,
    explain: question.explain ?? null,
    reveal: check.exact ? null : revealAnswer(question),
  });
}
