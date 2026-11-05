import type { Metadata } from "next";
import Link from "next/link";
import { Suspense } from "react";
import { connection } from "next/server";
import { CodexPanel, type CodexFilter } from "@/components/dashboard-sections";
import { Container, PageHeader, Skeleton } from "@/components/ui";
import { requireUser } from "@/lib/auth";
import { getUnlocked } from "@/lib/rewards";

export const metadata: Metadata = {
  title: "Codex",
  description: "Your achievements and what to aim for next.",
  alternates: { canonical: "/dashboard/codex" },
  robots: { index: false },
};

export default function CodexPage({ searchParams }: PageProps<"/dashboard/codex">) {
  return (
    <Container width="compact" className="py-8 sm:py-12">
      <Link href="/dashboard" className="inline-flex min-h-11 items-center text-sm text-muted hover:text-accent">Dashboard</Link>
      <PageHeader title="Codex" />
      <Suspense fallback={<Skeleton className="mt-6 h-96 w-full" />}><Body searchParams={searchParams} /></Suspense>
    </Container>
  );
}

async function Body({ searchParams }: { searchParams: PageProps<"/dashboard/codex">["searchParams"] }) {
  await connection();
  const user = await requireUser("/dashboard/codex");
  const unlocked = new Map((await getUnlocked(user.id)).map((r) => [r.id, r.unlockedAt]));
  const raw = (await searchParams).show;
  const asked = Array.isArray(raw) ? raw[0] : raw;
  // Earned first when there is something earned; otherwise show everything so a new learner sees what to aim for.
  const filter: CodexFilter = asked === "earned" || asked === "all" || asked === "locked" ? asked : unlocked.size > 0 ? "earned" : "all";
  return <CodexPanel unlocked={unlocked} filter={filter} />;
}
