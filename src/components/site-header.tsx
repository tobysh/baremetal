import Link from "next/link";
import { Suspense } from "react";
import { getCurrentUser } from "@/lib/auth";
import { logout } from "@/app/(auth)/actions";
import { LogoMark } from "./logo";
import { ThemeToggle } from "./theme-toggle";
import { LevelChip } from "./gamification";
import { levelFor } from "@/lib/gamification";
import { ButtonLink, Container, Skeleton } from "./ui";
import { SITE } from "@/lib/site";

export function SiteHeader() {
  return (
    <header className="sticky top-0 z-40 border-b border-border bg-bg/85 backdrop-blur supports-[backdrop-filter]:bg-bg/70">
      <Container className="flex min-h-14 flex-wrap items-center gap-x-3 gap-y-1 py-2 sm:h-14 sm:flex-nowrap sm:gap-6 sm:py-0">
        <Link href="/" className="flex items-center gap-2 font-semibold tracking-tight" aria-label={`${SITE.name} home`}>
          <LogoMark />
          <span className="hidden sm:inline">{SITE.name}</span>
        </Link>
        <nav aria-label="Main" className="flex items-center gap-1 text-sm">
          <Link href="/learn" className="rounded-md px-2 py-2 text-muted sm:px-3 hover:bg-surface-2 hover:text-text">
            Courses
          </Link>
          <Link href="/leaderboard" className="rounded-md px-2 py-2 text-muted sm:px-3 hover:bg-surface-2 hover:text-text">
            Leaderboard
          </Link>
        </nav>
        <div className="ml-auto flex flex-wrap items-center justify-end gap-1 sm:flex-nowrap sm:gap-2">
          <ThemeToggle />
          <Suspense fallback={<Skeleton className="h-9 w-40" />}>
            <AccountNav />
          </Suspense>
        </div>
      </Container>
    </header>
  );
}

async function AccountNav() {
  const user = await getCurrentUser();
  if (!user) {
    return (
      <>
        <ButtonLink href="/login" variant="ghost" size="sm">
          Sign in
        </ButtonLink>
        <ButtonLink href="/signup" size="sm">
          Start free
        </ButtonLink>
      </>
    );
  }
  return (
    <>
      <Link href={user.username ? `/u/${user.username}` : "/settings"} aria-label={`Your profile: level ${levelFor(user.xp).level}, ${user.xp} XP`} className="hidden sm:inline-flex">
        <LevelChip xp={user.xp} />
      </Link>
      <ButtonLink href="/dashboard" variant="ghost" size="sm">
        Dashboard
      </ButtonLink>
      {/* The admin link is a convenience only; /admin itself is protected on the server. */}
      {user.role === "admin" && (
        <ButtonLink href="/admin" variant="ghost" size="sm">
          Admin
        </ButtonLink>
      )}
      <ButtonLink href="/settings" variant="ghost" size="sm" aria-label="Account settings">
        Settings
      </ButtonLink>
      <form action={logout}>
        <button type="submit" className="rounded-md px-3 py-1.5 text-sm text-muted hover:bg-surface-2 hover:text-text">
          Sign out
        </button>
      </form>
    </>
  );
}
