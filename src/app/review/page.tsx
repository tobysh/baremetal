import type { Metadata } from "next";
import Link from "next/link";
import { Suspense } from "react";
import { connection } from "next/server";
import { ReviewCards, type ReviewCard } from "@/components/review-cards";
import { Container, PageHeader, Skeleton } from "@/components/ui";
import { requireUser } from "@/lib/auth";
import { getTracks, lessonPath, toPublicPractice } from "@/lib/curriculum";
import { getReviews } from "@/lib/progress";
import { pickQuestion, reviewDeck } from "@/lib/review-cards";

export const metadata: Metadata = {
  title: "Review questions",
  description: "Answer a question from each lesson that is ready for a second look.",
  alternates: { canonical: "/review" },
  robots: { index: false },
};

export default function ReviewPage() {
  return (
    <Container width="narrow" className="py-10 sm:py-12">
      <PageHeader title="Review questions" description="Answer from memory. A right answer pushes the lesson further out, a wrong one brings it back in a few days. Reviews give no XP." />
      <Suspense fallback={<Skeleton className="mt-6 h-72 w-full" />}><Cards /></Suspense>
    </Container>
  );
}

async function Cards() {
  await connection();
  const user = await requireUser("/review");
  const now = new Date();
  const lessonOf = new Map(getTracks().flatMap((t) => t.lessons.map((l) => [l.exercise.id, { t, l }] as const)));
  const cards: ReviewCard[] = (await getReviews(user.id))
    .filter((r) => r.dueAt <= now && lessonOf.has(r.exerciseId))
    .slice(0, 20)
    .map((r) => {
      const { t, l } = lessonOf.get(r.exerciseId)!;
      const deck = reviewDeck(l);
      const q = pickQuestion(deck.length, r);
      return {
        exerciseId: r.exerciseId,
        title: l.title,
        trackTitle: t.title,
        summary: l.summary,
        href: `${lessonPath(t.slug, l.slug)}?review=1`,
        language: t.language,
        // Lessons with no question get a "write it again" card that opens the coding review instead of a self-grade.
        question: q < 0 ? null : { q, practice: toPublicPractice(deck[q], l.exercise.id, l.difficulty, `${l.exercise.id}#card${q}`) },
      };
    });
  if (cards.length === 0) {
    return (
      <div className="panel mt-6 p-6">
        <p className="font-medium">Nothing is due.</p>
        <p className="mt-1 text-sm text-muted">Reviews appear here a few days after you pass a lesson.</p>
        <Link href="/dashboard" className="mt-4 inline-flex min-h-11 items-center rounded-lg border border-line-strong px-4 text-sm font-medium hover:bg-surface-2">Back to your dashboard</Link>
      </div>
    );
  }
  return <ReviewCards cards={cards} />;
}
