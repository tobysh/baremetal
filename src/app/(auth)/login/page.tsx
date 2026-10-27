import type { Metadata } from "next";
import Link from "next/link";
import { Suspense } from "react";
import { Skeleton } from "@/components/ui";
import { safeNext } from "@/lib/auth";
import { AuthShell } from "../auth-shell";
import { LoginForm } from "../auth-forms";

export const metadata: Metadata = {
  title: "Sign in",
  description: "Sign in to Baremetal to pick up your C and Rust lessons where you left off and chat with your AI tutor.",
  alternates: { canonical: "/login" },
};

export default function LoginPage({ searchParams }: PageProps<"/login">) {
  return (
    <AuthShell
      title="Welcome back"
      intro="Sign in to continue where you left off."
      footer={
        <>
          New here?{" "}
          <Link href="/signup" className="text-accent underline underline-offset-2">
            Create a free account
          </Link>
        </>
      }
    >
      <Suspense fallback={<FormSkeleton />}>
        <LoginBody searchParams={searchParams} />
      </Suspense>
    </AuthShell>
  );
}

async function LoginBody({ searchParams }: { searchParams: PageProps<"/login">["searchParams"] }) {
  const sp = await searchParams;
  const notice = sp.reset
    ? "Your password has been changed. Sign in with your new password."
    : sp.expired
      ? "Your sign-in attempt expired or had too many wrong codes. Please sign in again."
      : null;
  return (
    <div className="space-y-5">
      {notice && (
        <p role="status" className="rounded-lg bg-ok-soft px-4 py-3 text-sm">
          {notice}
        </p>
      )}
      <LoginForm next={safeNext(sp.next)} />
    </div>
  );
}

function FormSkeleton() {
  return (
    <div className="space-y-5" aria-busy="true">
      <Skeleton className="h-16" />
      <Skeleton className="h-16" />
      <Skeleton className="h-11" />
    </div>
  );
}
