import type { Metadata } from "next";
import Link from "next/link";
import { Suspense } from "react";
import { connection } from "next/server";
import { Card, Container, Skeleton } from "@/components/ui";
import { requireAdmin } from "@/lib/auth";
import { getStuckReport, top, type StuckRow } from "@/lib/stuck";

export const metadata: Metadata = {
  title: "Admin: where learners get stuck",
  description: "Lessons with the lowest pass rates, most attempts, most hints and most tutor use.",
  alternates: { canonical: "/admin/stuck" },
  robots: { index: false, follow: false },
};

export default function StuckPage() {
  return (
    <Container className="py-12">
      <h1 className="display text-3xl font-bold">Where learners get stuck</h1>
      <p className="mt-2 text-muted">
        <Link href="/admin" className="text-accent underline underline-offset-2">
          Back to analytics
        </Link>{" "}
        · aggregates only, all time
      </p>
      <Suspense fallback={<StuckSkeleton />}>
        <StuckBody />
      </Suspense>
    </Container>
  );
}

const pct = (n: number) => `${Math.round(n * 100)}%`;
const fixed = (n: number) => n.toFixed(1);

async function StuckBody() {
  await connection();
  await requireAdmin();
  const r = await getStuckReport();
  const withPasses = r.rows;
  const sections: { title: string; note: string; rows: StuckRow[]; cols: [string, (x: StuckRow) => string][] }[] = [
    {
      title: "Lowest pass rate",
      note: "Share of learners who started the lesson and have passed it.",
      rows: [...withPasses].sort((a, b) => a.passRate - b.passRate || b.started - a.started).slice(0, 10),
      cols: [["Pass rate", (x) => pct(x.passRate)], ["Passed / started", (x) => `${x.passed} / ${x.started}`]],
    },
    {
      title: "Most attempts before first pass",
      note: "Mean runs recorded at the moment of the first pass.",
      rows: top(r.rows.filter((x) => x.avgAttemptsToPass !== null), (x) => x.avgAttemptsToPass ?? 0),
      cols: [["Mean attempts", (x) => fixed(x.avgAttemptsToPass ?? 0)], ["Passed", (x) => String(x.passed)]],
    },
    {
      title: "Most hints used",
      note: "Mean hints opened per learner who started.",
      rows: top(r.rows.filter((x) => x.avgHints > 0), (x) => x.avgHints),
      cols: [["Hints per learner", (x) => fixed(x.avgHints)], ["Opened any", (x) => pct(x.hintShare)]],
    },
    {
      title: "Most tutor use",
      note: "Tutor questions asked about the lesson, and learners with a tutor thread.",
      rows: top(r.rows.filter((x) => x.tutorQuestions > 0), (x) => x.tutorQuestions),
      cols: [["Questions", (x) => String(x.tutorQuestions)], ["Learners", (x) => String(x.tutorLearners)]],
    },
  ];
  return (
    <div className="mt-8 space-y-6">
      <p className="text-sm text-muted">
        Lessons with fewer than {r.minLearners} learners are left out ({r.hidden} hidden), so no row describes one person. {r.rows.length} of {r.totalLessons} lessons are ranked.
      </p>
      <div className="grid gap-6 lg:grid-cols-2">
        {sections.map((s) => (
          <Card key={s.title} as="section" className="p-6">
            <h2 className="display-sm font-semibold">{s.title}</h2>
            <p className="mt-1 text-sm text-muted">{s.note}</p>
            {s.rows.length === 0 ? (
              <p className="mt-3 text-sm text-muted">No data yet.</p>
            ) : (
              <table className="mt-3 w-full text-sm">
                <caption className="sr-only">{s.title}</caption>
                <thead>
                  <tr className="text-left text-muted">
                    <th scope="col" className="py-1.5 font-medium">Lesson</th>
                    {s.cols.map(([h]) => (
                      <th scope="col" key={h} className="py-1.5 text-right font-medium">{h}</th>
                    ))}
                  </tr>
                </thead>
                <tbody>
                  {s.rows.map((x) => (
                    <tr key={x.id} className="border-t border-border">
                      <td className="py-1.5 pr-2">
                        <Link href={x.href} className="underline underline-offset-2 hover:text-accent">{x.title}</Link>
                        <span className="block text-xs text-muted">{x.track}</span>
                      </td>
                      {s.cols.map(([h, f]) => (
                        <td key={h} className="py-1.5 text-right tabular-nums">{f(x)}</td>
                      ))}
                    </tr>
                  ))}
                </tbody>
              </table>
            )}
          </Card>
        ))}
      </div>
    </div>
  );
}

function StuckSkeleton() {
  return (
    <div className="mt-8 grid gap-6 lg:grid-cols-2" aria-busy="true" aria-label="Loading report">
      {[0, 1, 2, 3].map((i) => (
        <Skeleton key={i} className="h-64" />
      ))}
    </div>
  );
}
