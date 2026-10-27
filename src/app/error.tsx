"use client";

import Link from "next/link";
import { Container } from "@/components/ui";

export default function ErrorPage({ error, reset }: { error: Error & { digest?: string }; reset: () => void }) {
  return (
    <Container className="max-w-2xl py-20 text-center">
      <title>Something went wrong | Baremetal</title>
      <p className="font-mono text-sm text-err">runtime error</p>
      <h1 className="mt-4 text-3xl font-bold tracking-tight">Something went wrong on our side</h1>
      <p className="mt-4 text-muted">It isn&apos;t your fault. Try again, and if it keeps happening, come back in a few minutes.</p>
      {error.digest && <p className="mt-2 font-mono text-xs text-muted">Reference: {error.digest}</p>}
      <div className="mt-8 flex justify-center gap-3">
        <button type="button" onClick={reset} className="inline-flex h-12 items-center rounded-lg bg-accent px-6 font-medium text-accent-ink">
          Try again
        </button>
        <Link href="/" className="inline-flex h-12 items-center rounded-lg border border-border px-6 font-medium">
          Go home
        </Link>
      </div>
    </Container>
  );
}
