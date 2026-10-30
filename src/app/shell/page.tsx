import type { Metadata } from "next";
import Link from "next/link";
import { Suspense } from "react";
import { connection } from "next/server";
import { Container, Skeleton } from "@/components/ui";
import { getUserDoc } from "@/lib/auth";
import { ShellLoader } from "./shell-loader";

export const metadata: Metadata = {
  title: "Shell",
  description: "A private Linux shell in your browser for the hands-on security labs.",
  alternates: { canonical: "/shell" },
  robots: { index: false },
};

export default function ShellPage() {
  return (
    <Container className="py-10 sm:py-12">
      <h1 className="display text-3xl font-bold leading-[1.1] sm:text-[34px]">Shell</h1>
      <p className="mt-3 max-w-2xl text-muted">
        A private Linux machine that exists only for your session. It has no internet access, nothing you type is stored, and it is destroyed when you leave or after 30 minutes.
      </p>
      <div className="mt-6">
        <Suspense fallback={<ShellSkeleton />}>
          <ShellBody />
        </Suspense>
      </div>
    </Container>
  );
}

function ShellSkeleton() {
  return (
    <div aria-busy="true" aria-label="Loading shell">
      <Skeleton className="h-[28rem] w-full rounded-2xl" />
    </div>
  );
}

async function ShellBody() {
  await connection();
  const user = await getUserDoc();
  if (!user) {
    return (
      <div className="panel p-6">
        <p>You need to be signed in to open a shell.</p>
        <Link className="mt-3 inline-block font-medium text-accent underline underline-offset-4" href="/login?next=/shell">Sign in</Link>
      </div>
    );
  }
  if (user.emailVerified === false) {
    return <div className="panel p-6"><p>Verify your email address to use the shell. Check your inbox for the verification link.</p></div>;
  }
  return <ShellLoader />;
}
