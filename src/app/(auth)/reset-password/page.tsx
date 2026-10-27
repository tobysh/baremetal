import type { Metadata } from "next";
import Link from "next/link";
import { Suspense } from "react";
import { Skeleton } from "@/components/ui";
import { AuthShell } from "../auth-shell";
import { ResetForm } from "../auth-forms";

export const metadata: Metadata = {
  title: "Choose a new password",
  description: "Choose a new password for your Baremetal account.",
  robots: { index: false },
  // The token is in the URL: never send it to other sites in the Referer header.
  referrer: "no-referrer",
};

export default function ResetPasswordPage({ searchParams }: PageProps<"/reset-password">) {
  return (
    <AuthShell title="Choose a new password" intro="After this, you'll be signed out everywhere and can sign in with the new password.">
      <Suspense fallback={<Skeleton className="h-64" />}>
        <ResetBody searchParams={searchParams} />
      </Suspense>
    </AuthShell>
  );
}

async function ResetBody({ searchParams }: { searchParams: PageProps<"/reset-password">["searchParams"] }) {
  const { token } = await searchParams;
  if (typeof token !== "string" || token.length < 20) {
    return (
      <p className="text-sm">
        This reset link is incomplete.{" "}
        <Link href="/forgot-password" className="text-accent underline underline-offset-2">
          Request a new one
        </Link>
        .
      </p>
    );
  }
  return <ResetForm token={token} />;
}
