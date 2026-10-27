import type { Metadata } from "next";
import Link from "next/link";
import { Suspense } from "react";
import { LevelChip } from "@/components/gamification";
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
    <Container className="max-w-3xl py-12">
      <h1 className="text-3xl font-bold tracking-tight">Leaderboard</h1>
      <p className="mt-2 text-muted">Earn XP by passing lessons: Easy 5, Medium 10, Hard 20, Impossible 40.</p>
      <form action="/leaderboard" role="search" className="mt-6 flex gap-2">
        <label htmlFor="q" className="sr-only">
          Find a learner by username
        </label>
        <Suspense fallback={<input disabled className="h-11 flex-1 rounded-lg border border-border bg-bg px-3" />}>
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
      className="h-11 flex-1 rounded-lg border border-border bg-bg px-3 placeholder:text-muted"
    />
  );
}

async function Results({ searchParams }: { searchParams: PageProps<"/leaderboard">["searchParams"] }) {
  const { q } = await searchParams;
  const me = await getCurrentUser();
  if (typeof q === "string" && q.trim()) {
    const found = await searchUsers(q);
    return (
      <section aria-labelledby="results" className="mt-8">
        <h2 id="results" className="text-sm font-medium text-muted">
          {found.length ? `Learners matching “${q.trim()}”` : `No public profiles match “${q.trim()}”.`}
        </h2>
        {found.length > 0 && (
          <ul className="mt-3 divide-y divide-border overflow-hidden rounded-xl border border-border bg-surface">
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
      <h2 id="top" className="text-sm font-medium text-muted">
        Top learners
      </h2>
      {rows.length === 0 ? (
        <Card className="mt-3 p-6 text-sm text-muted">No one is on the board yet. Pass a lesson to be first.</Card>
      ) : (
        <ol className="mt-3 divide-y divide-border overflow-hidden rounded-xl border border-border bg-surface">
          {rows.map((r) => (
            <Row key={r.username} rank={r.rank} username={r.username} xp={r.xp} isMe={me?.username === r.username} />
          ))}
        </ol>
      )}
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
      <Link href={`/u/${username}`} className={`flex items-center gap-4 px-4 py-3 hover:bg-surface-2 ${isMe ? "bg-accent-soft" : ""}`}>
        {rank !== undefined && <span className="w-8 text-right font-mono text-sm text-muted tabular-nums">{rank}</span>}
        <span className="flex-1 truncate font-medium">
          {username}
          {isMe && <span className="ml-2 text-xs font-normal text-muted">(you)</span>}
          <span className="block text-xs font-normal text-muted">{l.title}</span>
        </span>
        <LevelChip xp={xp} />
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
