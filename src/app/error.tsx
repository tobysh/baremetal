"use client";

import { Button, ButtonLink, Container } from "@/components/ui";

export default function ErrorPage({ error, reset }: { error: Error & { digest?: string }; reset: () => void }) {
  return (
    <Container className="max-w-2xl py-20 text-center">
      <title>Something went wrong | Baremetal</title>
      <p className="font-mono text-sm text-err">runtime error</p>
      <h1 className="display mt-4 text-3xl font-bold">Something went wrong on our side</h1>
      <p className="mt-4 text-muted">It isn&apos;t your fault. Try again, and if it keeps happening, come back in a few minutes.</p>
      {error.digest && <p className="mt-2 font-mono text-xs text-muted">Reference: {error.digest}</p>}
      <div className="mt-8 flex flex-wrap justify-center gap-3">
        <Button type="button" onClick={reset} size="lg">
          Try again
        </Button>
        <ButtonLink href="/" size="lg" variant="secondary">
          Go home
        </ButtonLink>
      </div>
    </Container>
  );
}
