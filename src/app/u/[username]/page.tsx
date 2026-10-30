import type { Metadata } from "next";
import Link from "next/link";
import { notFound } from "next/navigation";
import { connection } from "next/server";
import { Suspense } from "react";
import { DifficultyBadge, LevelProgress } from "@/components/gamification";
import { Card, Container, ProgressBar, Skeleton } from "@/components/ui";
import { getCurrentUser } from "@/lib/auth";
import { getPublicProfile } from "@/lib/community";
import { USERNAME_RE, levelFor, titleBlurb } from "@/lib/gamification";
import { Seal } from "@/components/seals";

type Params = PageProps<"/u/[username]">["params"];

// One sample value so the shell prerenders; other usernames render on request.
export function generateStaticParams() {
  return [{ username: "example" }];
}

export async function generateMetadata({ params }: PageProps<"/u/[username]">): Promise<Metadata> {
  const { username } = await params;
  const name = username.toLowerCase();
  if (!USERNAME_RE.test(name)) notFound();
  return {
    title: `${name} — learner profile`,
    description: `${name}'s level, XP and completed lessons on Baremetal.`,
    alternates: { canonical: `/u/${name}` },
    robots: { index: false, follow: true },
  };
}

export default async function ProfilePage({ params }: PageProps<"/u/[username]">) {
  // Malformed names 404 here, before any streaming starts, so they get a real 404 status.
  // Well-formed but unknown names can only be detected inside <Suspense> (needs the DB).
  const name = (await params).username.toLowerCase();
  if (!USERNAME_RE.test(name)) notFound();
  return (
    <Container className="max-w-3xl py-10 sm:py-14">
      <Suspense fallback={<ProfileSkeleton />}>
        <Profile params={params} />
      </Suspense>
    </Container>
  );
}

async function Profile({ params }: { params: Params }) {
  await connection(); // the DB driver reads the clock, so wait for a real request
  const name = (await params).username.toLowerCase();
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
          <h1 className="display break-words text-3xl font-bold sm:text-4xl">{profile.username}</h1>
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

      <Card className="mt-8 p-6 sm:p-8">
        <LevelProgress xp={profile.xp} />
        <p className="mt-3 text-sm text-text-2">{levelFor(profile.xp).title}: {titleBlurb(levelFor(profile.xp).level)}</p>
        {profile.bestRhythm > 0 && <p className="mt-1 text-sm text-muted">Best weekly rhythm: {profile.bestRhythm} {profile.bestRhythm === 1 ? "week" : "weeks"} in a row.</p>}
      </Card>

      <h2 className="display-sm mt-10 text-lg font-bold">
        Courses <span className="text-sm font-normal text-muted">· {profile.completed} of {profile.totalLessons} lessons</span>
      </h2>
      <ul className="mt-3 grid gap-3 sm:grid-cols-2">
        {profile.tracks.map((t) => (
          <Card as="li" key={t.slug} className="p-5">
            <div className="flex flex-wrap items-baseline justify-between gap-3 text-sm">
              <Link href={`/learn/${t.slug}`} className="font-medium hover:text-accent">
                {t.title}
              </Link>
              <span className="shrink-0 text-muted tabular-nums">
                {t.done}/{t.total}
              </span>
            </div>
            {(t.unaided > 0 || t.challenges > 0) && (
              <p className="mt-2 flex flex-wrap gap-x-4 gap-y-1 text-xs text-muted">
                {t.unaided > 0 && <span className="inline-flex items-center gap-1.5"><Seal kind="unaided" earned size={14} /> {t.unaided} unaided</span>}
                {t.challenges > 0 && <span className="inline-flex items-center gap-1.5"><Seal kind="challenge" earned size={14} /> {t.challenges} {t.challenges === 1 ? "challenge" : "challenges"}</span>}
              </p>
            )}
            <div className="mt-2">
              <ProgressBar value={t.done / t.total} label={`${t.title} progress`} />
            </div>
          </Card>
        ))}
      </ul>

      <h2 className="display-sm mt-10 text-lg font-bold">
        Achievements <span className="text-sm font-normal text-muted">· {profile.achievements.length}</span>
      </h2>
      {profile.achievements.length === 0 ? (
        <p className="mt-3 text-sm text-muted">No achievements yet. They show up here as they are earned.</p>
      ) : (
        <ul className="mt-3 grid gap-3 sm:grid-cols-2">
          {profile.achievements.map((a) => (
            <Card as="li" key={a.name} className="p-4">
              <p className="font-medium">{a.name}</p>
              <p className="mt-0.5 text-sm text-muted">{a.description}</p>
            </Card>
          ))}
        </ul>
      )}

      <h2 className="display-sm mt-10 text-lg font-bold">Recently completed</h2>
      {profile.recent.length === 0 ? (
        <p className="mt-3 text-sm text-muted">No lessons completed yet. Finished lessons will appear here.</p>
      ) : (
        <ul className="mt-3 divide-y divide-border overflow-hidden panel">
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
