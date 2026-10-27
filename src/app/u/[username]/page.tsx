import type { Metadata } from "next";
import Link from "next/link";
import { notFound } from "next/navigation";
import { Suspense } from "react";
import { DifficultyBadge, LevelProgress } from "@/components/gamification";
import { Card, Container, ProgressBar, Skeleton } from "@/components/ui";
import { getCurrentUser } from "@/lib/auth";
import { getPublicProfile } from "@/lib/community";
import { USERNAME_RE } from "@/lib/gamification";

type Params = PageProps<"/u/[username]">["params"];

export async function generateMetadata({ params }: PageProps<"/u/[username]">): Promise<Metadata> {
  const { username } = await params;
  const name = decodeURIComponent(username).toLowerCase();
  return {
    title: `${name} — learner profile`,
    description: `${name}'s level, XP and completed C and Rust lessons on Baremetal.`,
    alternates: { canonical: `/u/${name}` },
    robots: { index: false, follow: true },
  };
}

export default function ProfilePage({ params }: PageProps<"/u/[username]">) {
  return (
    <Container className="max-w-3xl py-12">
      <Suspense fallback={<ProfileSkeleton />}>
        <Profile params={params} />
      </Suspense>
    </Container>
  );
}

async function Profile({ params }: { params: Params }) {
  const name = decodeURIComponent((await params).username).toLowerCase();
  if (!USERNAME_RE.test(name)) notFound();
  const [profile, me] = await Promise.all([getPublicProfile(name), getCurrentUser()]);
  if (!profile) notFound();
  const isMe = me?.username === profile.username;
  return (
    <>
      <nav aria-label="Breadcrumb" className="text-sm text-muted">
        <Link href="/leaderboard" className="hover:text-text">
          Leaderboard
        </Link>{" "}
        / <span aria-current="page">{profile.username}</span>
      </nav>
      <div className="mt-4 flex flex-wrap items-end justify-between gap-4">
        <div>
          <h1 className="text-3xl font-bold tracking-tight">{profile.username}</h1>
          <p className="mt-1 text-sm text-muted">
            Rank #{profile.rank} · learning since {profile.memberSince.toLocaleDateString("en-GB", { month: "long", year: "numeric" })}
          </p>
        </div>
        {isMe && (
          <Link href="/settings" className="text-sm text-accent underline underline-offset-2">
            Edit profile
          </Link>
        )}
      </div>

      <Card className="mt-8 p-6">
        <LevelProgress xp={profile.xp} />
      </Card>

      <h2 className="mt-10 text-lg font-semibold">
        Courses <span className="text-sm font-normal text-muted">· {profile.completed} of {profile.totalLessons} lessons</span>
      </h2>
      <ul className="mt-3 grid gap-3 sm:grid-cols-2">
        {profile.tracks.map((t) => (
          <Card as="li" key={t.slug} className="p-4">
            <div className="flex items-baseline justify-between gap-3 text-sm">
              <Link href={`/learn/${t.slug}`} className="font-medium hover:text-accent">
                {t.title}
              </Link>
              <span className="text-muted tabular-nums">
                {t.done}/{t.total}
              </span>
            </div>
            <div className="mt-2">
              <ProgressBar value={t.done / t.total} label={`${t.title} progress`} />
            </div>
          </Card>
        ))}
      </ul>

      <h2 className="mt-10 text-lg font-semibold">Recently completed</h2>
      {profile.recent.length === 0 ? (
        <p className="mt-3 text-sm text-muted">No lessons completed yet.</p>
      ) : (
        <ul className="mt-3 divide-y divide-border overflow-hidden rounded-xl border border-border bg-surface">
          {profile.recent.map((r) => (
            <li key={`${r.trackTitle}-${r.title}`} className="flex items-center gap-4 px-4 py-3">
              <span className="flex-1">
                <span className="font-medium">{r.title}</span>
                <span className="block text-xs text-muted">{r.trackTitle}</span>
              </span>
              <DifficultyBadge difficulty={r.difficulty} />
            </li>
          ))}
        </ul>
      )}
    </>
  );
}

function ProfileSkeleton() {
  return (
    <div aria-busy="true" aria-label="Loading profile" className="space-y-4">
      <Skeleton className="h-4 w-40" />
      <Skeleton className="h-9 w-56" />
      <Skeleton className="h-4 w-64" />
      <Skeleton className="mt-8 h-32" />
      <div className="grid gap-3 pt-6 sm:grid-cols-2">
        {[0, 1, 2, 3].map((i) => (
          <Skeleton key={i} className="h-20" />
        ))}
      </div>
    </div>
  );
}
