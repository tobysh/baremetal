import { NextResponse } from "next/server";
import { getCurrentUser } from "@/lib/auth";
import { checkPractice, findExercise, revealAnswer, toPublicPractice } from "@/lib/curriculum";
import { jsonError, readJson, sameOrigin } from "@/lib/http";
import { rateLimit } from "@/lib/rate-limit";
import { isReviewDue, logRetry, retryQuestion, reviewDeck, settleReviewCard, skipReview } from "@/lib/review-cards";

/**
 * Graded review cards: `{ exerciseId, q, submission }` answers question `q` of that lesson's deck. The answer is
 * checked here and only revealed once the schedule has moved, so a due card can be answered once: right moves the
 * lesson to its next interval, wrong starts it over. No XP, no Codex credit. Only exact answers count.
 *
 * `{ exerciseId, action: "skip" }` postpones a due lesson that has no question deck to tomorrow.
 * `{ exerciseId, q, submission, retry: true }` is the extra attempt after a miss: logged, never scheduled.
 */
export async function POST(req: Request) {
  if (!sameOrigin(req)) return jsonError(403, "Cross-site request blocked.");
  const user = await getCurrentUser();
  if (!user) return jsonError(401, "Sign in to save your reviews.");
  const body = await readJson<{ exerciseId?: string; q?: number; submission?: unknown; action?: string; retry?: boolean }>(req, 16 * 1024);
  if (!body || typeof body.exerciseId !== "string") return jsonError(400, "Invalid request.");
  const found = findExercise(body.exerciseId);
  if (!found || found.isChallenge) return jsonError(404, "Unknown exercise.");
  const deck = reviewDeck(found.lesson);
  if (!(await rateLimit(`review-card:${user.id}`, 120, 60))) return jsonError(429, "Too many requests.");

  if (body.action === "skip") {
    if (deck.length > 0) return jsonError(400, "This lesson has a question; answer it instead.");
    if (!(await skipReview(user.id, found.exercise.id, user.timezone))) return jsonError(409, "This review is not due, or you already answered it.");
    return NextResponse.json({ ok: true });
  }

  if (!Number.isInteger(body.q) || body.submission === undefined) return jsonError(400, "Invalid request.");
  const question = deck[body.q!];
  if (!question) return jsonError(404, "Unknown question.");

  if (body.retry === true) {
    const check = checkPractice(question, body.submission);
    if (!check.valid) return jsonError(400, check.error);
    if (!(await logRetry(user.id, found.exercise.id, deck.length, { correct: check.exact, partial: check.partial, questionIndex: body.q! }))) return jsonError(409, "There is nothing to retry.");
    return NextResponse.json({ ok: true, correct: check.exact, feedback: check.exact ? null : check.feedback ?? null, explain: question.explain ?? null, reveal: check.exact ? null : revealAnswer(question), retry: null });
  }

  if (!(await isReviewDue(user.id, found.exercise.id))) return jsonError(409, "This review is not due, or you already answered it.");
  const check = checkPractice(question, body.submission);
  if (!check.valid) return jsonError(400, check.error);
  const event = await settleReviewCard(user.id, found.exercise.id, { correct: check.exact, partial: check.partial, questionIndex: body.q! });
  if (!event) return jsonError(409, "This review is not due, or you already answered it.");
  // After a miss, one more question from the same deck. A one-question deck is brought back at the end of the session instead (the client does that).
  const again = check.exact ? null : retryQuestion(deck.length, body.q!);
  return NextResponse.json({
    ok: true,
    correct: check.exact,
    feedback: check.exact ? null : check.feedback ?? null,
    explain: question.explain ?? null,
    reveal: check.exact ? null : revealAnswer(question),
    retry: again === null ? null : { q: again, practice: toPublicPractice(deck[again], found.exercise.id, found.lesson.difficulty, `${found.exercise.id}#card${again}`) },
  });
}
