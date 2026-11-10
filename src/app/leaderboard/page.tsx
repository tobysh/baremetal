import type { Metadata } from "next";
import Link from "next/link";
import { connection } from "next/server";
import { Suspense } from "react";
import { Card, Container, Skeleton } from "@/components/ui";
import { getCurrentUser } from "@/lib/auth";
import { getLeaderboard, searchUsers } from "@/lib/community";
import { levelFor } from "@/lib/gamification";

export const metadata: Metadata = {
  title: "Leaderboard",
  description: "The Baremetal learners with the most XP, and a search to find other learners by username.",
  alternates: { canonical: "/leaderboard" },
  // Usernames belong to learners; keep them out of search engines.
  robots: { index: false, follow: true },
};

export default function LeaderboardPage({ searchParams }: PageProps<"/leaderboard">) {
  return (
    <Container width="compact" className="py-10 sm:py-14">
      <h1 className="display text-3xl font-bold sm:text-4xl">Leaderboard</h1>
      <p className="mt-3 max-w-xl text-muted">Small steps add up. See what learners are building, one lesson at a time.</p>
      <details className="mt-3 text-sm text-muted">
        <summary className="cursor-pointer font-medium text-text-2 hover:text-accent">How XP works</summary>
        <p className="mt-2">XP per lesson: Easy 5 · Medium 10 · Hard 20 · Impossible 40. Each lesson earns XP once, the first time you pass it.</p>
      </details>
      <form action="/leaderboard" role="search" className="mt-6 flex gap-2">
        <label htmlFor="q" className="sr-only">
          Find a learner by username
        </label>
        <Suspense fallback={<input disabled className="h-11 min-w-0 flex-1 rounded-lg border border-field bg-bg px-3" />}>
          <SearchInput searchParams={searchParams} />
        </Suspense>
        <button type="submit" className="h-11 rounded-lg border border-border px-4 text-sm font-medium hover:bg-surface-2">
          Search
        </button>
      </form>
      <Suspense fallback={<ListSkeleton />}>
        <Results searchParams={searchParams} />
      </Suspense>
    </Container>
  );
}

async function SearchInput({ searchParams }: { searchParams: PageProps<"/leaderboard">["searchParams"] }) {
  const { q } = await searchParams;
  return (
    <input
      id="q"
      name="q"
      type="search"
      defaultValue={typeof q === "string" ? q : ""}
      placeholder="Find a learner by username"
      autoComplete="off"
      autoCapitalize="none"
      spellCheck={false}
      maxLength={20}
      className="h-11 min-w-0 flex-1 rounded-lg border border-field bg-bg px-3 placeholder:text-muted"
    />
  );
}

async function Results({ searchParams }: { searchParams: PageProps<"/leaderboard">["searchParams"] }) {
  // MongoDB initialization uses request-time clocks; keep it out of the static shell.
  await connection();
  const { q } = await searchParams;
  const me = await getCurrentUser();
  if (typeof q === "string" && q.trim()) {
    const found = await searchUsers(q);
    return (
      <section aria-labelledby="results" className="mt-8">
        <h2 id="results" className="display-sm text-lg font-bold">
          {found.length ? `Learners matching “${q.trim()}”` : `No public profiles match “${q.trim()}”.`}
        </h2>
        {found.length === 0 && <p className="mt-2 text-sm text-muted">Usernames are lowercase letters, digits, - and _. Try the first few letters, or go back to the board.</p>}
        {found.length > 0 && (
          <ul className="mt-3 divide-y divide-border overflow-hidden panel">
            {found.map((u) => (
              <Row key={u.username} username={u.username} xp={u.xp} isMe={me?.username === u.username} />
            ))}
          </ul>
        )}
        <Link href="/leaderboard" className="mt-4 inline-block text-sm text-accent underline underline-offset-2">
          Back to the leaderboard
        </Link>
      </section>
    );
  }
  const rows = await getLeaderboard(50);
  return (
    <section aria-labelledby="top" className="mt-8">
      <h2 id="top" className="display-sm text-lg font-bold">
        Top learners
      </h2>
      {rows.length === 0 ? (
        <Card className="mt-3 p-6 text-sm text-muted">No one is on the board yet, so the first place is open. <Link href="/dashboard" className="text-accent underline underline-offset-2">Pass a lesson</Link> and set a public username in settings to claim it.</Card>
      ) : (
        <ol className="mt-3 divide-y divide-border overflow-hidden panel">
          {rows.map((r) => (
            <Row key={r.username} rank={r.rank} username={r.username} xp={r.xp} isMe={me?.username === r.username} />
          ))}
        </ol>
      )}
      {rows.length > 0 && rows.length < 5 && <p className="mt-4 text-sm text-muted">Only a few learners are on the board so far. Pass a lesson and choose a public username in settings to join them.</p>}
      {me && !me.username && (
        <p className="mt-4 text-sm text-muted">
          <Link href="/settings" className="text-accent underline underline-offset-2">
            Choose a username
          </Link>{" "}
          to appear here.
        </p>
      )}
    </section>
  );
}

function Row({ rank, username, xp, isMe }: { rank?: number; username: string; xp: number; isMe: boolean }) {
  const l = levelFor(xp);
  return (
    <li>
      <Link href={`/u/${username}`} className={`grid grid-cols-[2rem_minmax(0,1fr)_auto] items-center gap-x-3 px-4 py-4 transition-colors hover:bg-surface-2 sm:grid-cols-[2rem_minmax(0,1fr)_9rem_6rem] ${isMe ? "bg-accent-soft" : ""}`}>
        <span className="text-right font-mono text-sm text-muted tabular-nums">{rank ?? ""}</span>
        <span className="min-w-0 truncate font-medium">
          {username}
          {isMe && <span className="ml-2 text-xs font-normal text-muted">(you)</span>}
        </span>
        <span className="hidden text-sm text-muted sm:block">{l.title}</span>
        <span className="text-right text-sm font-medium tabular-nums">{xp} XP</span>
      </Link>
    </li>
  );
}

function ListSkeleton() {
  return (
    <div className="mt-8 space-y-px" aria-busy="true" aria-label="Loading leaderboard">
      {Array.from({ length: 8 }, (_, i) => (
        <Skeleton key={i} className="h-14 rounded-none first:rounded-t-xl last:rounded-b-xl" />
      ))}
    </div>
  );
}
