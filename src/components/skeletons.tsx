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
