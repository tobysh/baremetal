import type { Metadata } from "next";
import Link from "next/link";
import { AuthShell } from "../../auth-shell";
import { VerifyForm } from "../../auth-forms";

export const metadata: Metadata = {
  title: "Two-step verification",
  description: "Confirm it's you with the code from your authenticator app.",
  robots: { index: false },
};

export default function VerifyPage() {
  return (
    <AuthShell
      title="Two-step verification"
      intro="Your account is protected with two-factor authentication."
      footer={
        <Link href="/login" className="underline underline-offset-2">
          Start over
        </Link>
      }
    >
      <VerifyForm />
    </AuthShell>
  );
}
