import type { Metadata } from "next";
import Link from "next/link";
import { Suspense } from "react";
import { connection } from "next/server";
import { ObjectId } from "mongodb";
import { StatsPanel, type MonthXp } from "@/components/dashboard-sections";
import { Container, PageHeader, Skeleton } from "@/components/ui";
import { requireUser } from "@/lib/auth";
import { getTracks } from "@/lib/curriculum";
import { getDb } from "@/lib/db";
import { isChallengeId } from "@/lib/gamification";
import type { XpEventDoc } from "@/lib/models";
import { getProgressMap, getReviews } from "@/lib/progress";
import { buildContext } from "@/lib/rewards-context";

export const metadata: Metadata = {
  title: "Your numbers",
  description: "Monthly XP, seals and progress by course. Private to you.",
  alternates: { canonical: "/dashboard/stats" },
  robots: { index: false },
};

export default function StatsPage() {
  return (
    <Container width="compact" className="py-8 sm:py-12">
      <Link href="/dashboard" className="inline-flex min-h-11 items-center text-sm text-muted hover:text-accent">Dashboard</Link>
      <PageHeader title="Your numbers" />
      <Suspense fallback={<Skeleton className="mt-6 h-96 w-full" />}><Body /></Suspense>
    </Container>
  );
}

async function Body() {
  await connection();
  const user = await requireUser("/dashboard/stats");
  const [progress, reviews, xpEvents] = await Promise.all([
    getProgressMap(user.id),
    getReviews(user.id),
    (await getDb()).collection<XpEventDoc>("xp_events").find({ userId: new ObjectId(user.id) }, { projection: { xp: 1, at: 1 } }).toArray(),
  ]);
  const tracks = getTracks();
  const now = new Date();
  const ctx = buildContext(tracks, [...progress.values()], reviews, { xp: user.xp, timezone: user.timezone ?? "UTC", weeklyGoal: user.weeklyGoal }, now);
  const months: MonthXp[] = Array.from({ length: 6 }, (_, i) => {
    const d = new Date(Date.UTC(now.getUTCFullYear(), now.getUTCMonth() - (5 - i), 1));
    const next = new Date(Date.UTC(d.getUTCFullYear(), d.getUTCMonth() + 1, 1));
    return { label: d.toLocaleDateString("en-GB", { month: "short", timeZone: "UTC" }), xp: xpEvents.filter((e) => e.at >= d && e.at < next).reduce((n, e) => n + e.xp, 0) };
  });
  const passedDocs = [...progress.values()].filter((p) => p.status === "passed" && !isChallengeId(p.exerciseId)).sort((a, b) => +(b.passedAt ?? 0) - +(a.passedAt ?? 0));
  const avgHints = (docs: typeof passedDocs) => docs.reduce((n, p) => n + (p.hintsUsedAtPass ?? 0), 0) / Math.max(1, docs.length);
  const earlier = passedDocs.slice(10, 20);
  return (
    <StatsPanel
      months={months}
      seals={{ passed: ctx.passed.size, unaided: [...ctx.passed.values()].filter((p) => p.unaided).length, challenge: ctx.challenges.size }}
      firstTry={[...ctx.passed.values()].filter((p) => p.attemptsAtPass === 1).length}
      hints={{ recent: avgHints(passedDocs.slice(0, 10)), earlier: earlier.length ? avgHints(earlier) : null }}
      tracks={tracks.map((t) => ({ title: t.title, done: t.lessons.filter((l) => ctx.passed.has(l.exercise.id)).length, total: t.lessons.length }))}
    />
  );
}
