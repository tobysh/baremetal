import Link from "next/link";
import { Suspense } from "react";
import { getCurrentUser } from "@/lib/auth";
import { getProgressMap } from "@/lib/progress";
import { computeRhythm } from "@/lib/rhythm";
import { LogoMark } from "./logo";
import { HeaderNavigation } from "./header-navigation";
import { Container, Skeleton } from "./ui";
import { SITE } from "@/lib/site";

export function SiteHeader() {
  return (
    <header className="sticky top-0 z-40 border-b border-border bg-bg">
      <Container className="flex h-16 items-center gap-5 sm:gap-8">
        <Link href="/" className="flex shrink-0 items-center gap-2.5 display text-[17px] font-bold" aria-label={`${SITE.name} home`}>
          <LogoMark />
          <span>{SITE.name}</span>
        </Link>
        <Suspense fallback={<Skeleton className="ml-auto h-10 w-24" />}>
          <AccountNavigation />
        </Suspense>
      </Container>
    </header>
  );
}

async function AccountNavigation() {
  const user = await getCurrentUser();
  let rhythm: { goal: number; thisWeek: number } | null = null;
  if (user && user.weeklyGoal > 0) {
    // Same rule as the dashboard: first passes that earned XP, counted by week in the learner's zone.
    const passes = [...(await getProgressMap(user.id)).values()].filter((p) => p.status === "passed" && !p.xpWithheld && p.passedAt);
    const r = computeRhythm(passes.map((p) => p.passedAt as Date), user.weeklyGoal, user.timezone ?? "UTC");
    rhythm = { goal: r.goal, thisWeek: r.thisWeek };
  }
  return <HeaderNavigation user={user ? { name: user.name, username: user.username, xp: user.xp, role: user.role } : null} rhythm={rhythm} />;
}
