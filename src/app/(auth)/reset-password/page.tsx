import type { Metadata } from "next";
import { Suspense } from "react";
import { Skeleton } from "@/components/ui";
import { AuthShell } from "../auth-shell";
import { FragmentResetForm } from "../fragment-forms";

export const metadata: Metadata = {
  title: "Choose a new password",
  description: "Choose a new password for your Baremetal account.",
  robots: { index: false },
  // The token is in the URL: never send it to other sites in the Referer header.
  referrer: "no-referrer",
};

export default function ResetPasswordPage() {
  return (
    <AuthShell title="Choose a new password" intro="After this, you'll be signed out everywhere and can sign in with the new password.">
      <Suspense fallback={<Skeleton className="h-64" />}>
        <FragmentResetForm />
      </Suspense>
    </AuthShell>
  );
}
