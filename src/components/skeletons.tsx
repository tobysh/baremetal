import { Container, Skeleton } from "./ui";

// Shown instantly while navigating between lessons.
export function LessonSkeleton() {
  return (
    <div className="mx-auto w-full max-w-[96rem] px-4 py-6 sm:px-6" aria-busy="true" aria-label="Loading lesson">
      <Skeleton className="mb-5 h-12 w-64" />
      <div className="grid gap-6 lg:grid-cols-[minmax(0,1fr)_minmax(0,1.15fr)]">
        <div className="space-y-4">
          <Skeleton className="h-4 w-56" />
          <div className="flex gap-2">
            <Skeleton className="h-6 w-28 rounded-full" />
            <Skeleton className="h-6 w-16 rounded-full" />
          </div>
          <Skeleton className="h-9 w-2/3" />
          <Skeleton className="h-4 w-full" />
          <div className="space-y-2 pt-4">
            {[0, 1, 2, 3, 4].map((i) => (
              <Skeleton key={i} className={i === 4 ? "h-4 w-3/4" : "h-4"} />
            ))}
          </div>
          <Skeleton className="h-40 w-full" />
          <div className="space-y-2">
            <Skeleton className="h-4" />
            <Skeleton className="h-4 w-4/5" />
          </div>
        </div>
        <Skeleton className="hidden h-[36rem] rounded-xl lg:block lg:h-[calc(100dvh-10rem)]" />
      </div>
    </div>
  );
}

export function TrackSkeleton() {
  return (
    <Container className="py-12">
      <div aria-busy="true" aria-label="Loading course">
        <Skeleton className="h-4 w-48" />
        <div className="mt-6 grid gap-10 lg:grid-cols-[1fr_20rem]">
          <div className="space-y-4">
            <div className="flex gap-2">
              <Skeleton className="h-6 w-24 rounded-full" />
              <Skeleton className="h-6 w-12 rounded-full" />
            </div>
            <Skeleton className="h-10 w-2/3" />
            <Skeleton className="h-5 w-full" />
            <Skeleton className="h-5 w-4/5" />
            <div className="space-y-px pt-8">
              {[0, 1, 2, 3, 4, 5].map((i) => (
                <Skeleton key={i} className="h-20 rounded-none first:rounded-t-xl last:rounded-b-xl" />
              ))}
            </div>
          </div>
          <Skeleton className="h-48 rounded-xl" />
        </div>
      </div>
    </Container>
  );
}

export function DashboardSkeleton() {
  return (
    <div aria-busy="true" aria-label="Loading your dashboard" className="flex flex-col gap-6">
      <div className="flex flex-wrap gap-6">
        <Skeleton className="h-72 min-w-0 flex-[999_1_600px]" />
        <Skeleton className="h-72 min-w-0 flex-[1_1_320px]" />
      </div>
      <Skeleton className="h-80" />
    </div>
  );
}

/** Title block shared by the simple page skeletons. */
function TitleSkeleton({ description = true }: { description?: boolean }) {
  return (
    <div className="space-y-3">
      <Skeleton className="h-9 w-64 sm:h-10" />
      {description && <Skeleton className="h-5 w-full max-w-xl" />}
    </div>
  );
}

export function LeaderboardSkeleton() {
  return (
    <Container width="narrow" className="py-10 sm:py-14">
      <div aria-busy="true" aria-label="Loading leaderboard">
        <TitleSkeleton />
        <Skeleton className="mt-6 h-11 w-full rounded-lg" />
        <div className="mt-8 space-y-px">
          {Array.from({ length: 8 }, (_, i) => (
            <Skeleton key={i} className="h-14 rounded-none first:rounded-t-xl last:rounded-b-xl" />
          ))}
        </div>
      </div>
    </Container>
  );
}

export function ProfileLoadingSkeleton() {
  return (
    <Container width="narrow" className="py-10 sm:py-14">
      <div aria-busy="true" aria-label="Loading profile" className="space-y-3">
        <Skeleton className="h-4 w-40" />
        <Skeleton className="h-9 w-56" />
        <Skeleton className="h-4 w-64" />
        <Skeleton className="mt-8 h-32" />
        <div className="grid gap-3 pt-3 sm:grid-cols-2">
          {[0, 1, 2, 3].map((i) => (
            <Skeleton key={i} className="h-20" />
          ))}
        </div>
      </div>
    </Container>
  );
}

export function SettingsLoadingSkeleton() {
  return (
    <Container width="narrow" className="py-10 sm:py-14">
      <div aria-busy="true" aria-label="Loading settings">
        <TitleSkeleton description={false} />
        <div className="mt-6 flex gap-6 border-b border-border pb-3">
          <Skeleton className="h-5 w-32" />
          <Skeleton className="h-5 w-20" />
        </div>
        <div className="mt-8 space-y-8">
          <Skeleton className="h-64" />
          <Skeleton className="h-80" />
          <Skeleton className="h-40" />
        </div>
      </div>
    </Container>
  );
}

export function ShellLoadingSkeleton() {
  return (
    <Container className="py-10 sm:py-12">
      <div aria-busy="true" aria-label="Loading shell">
        <TitleSkeleton />
        <Skeleton className="mt-6 h-[28rem] w-full rounded-2xl" />
      </div>
    </Container>
  );
}

export function LearnSkeleton() {
  return (
    <Container className="py-8 sm:py-12">
      <div aria-busy="true" aria-label="Loading courses">
        <TitleSkeleton />
        <div className="mt-8 flex flex-wrap gap-2">
          {[0, 1, 2, 3, 4].map((i) => (
            <Skeleton key={i} className="h-11 w-28 rounded-lg" />
          ))}
        </div>
        <div className="mt-10 grid gap-4 sm:grid-cols-2">
          {[0, 1, 2, 3].map((i) => (
            <Skeleton key={i} className="h-48 rounded-2xl" />
          ))}
        </div>
      </div>
    </Container>
  );
}
