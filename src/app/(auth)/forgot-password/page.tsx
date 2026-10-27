import type { Metadata } from "next";
import Link from "next/link";
import { AuthShell } from "../auth-shell";
import { ForgotForm } from "../auth-forms";

export const metadata: Metadata = {
  title: "Reset your password",
  description: "Forgotten your Baremetal password? Enter your email and we'll send you a link to choose a new one.",
  alternates: { canonical: "/forgot-password" },
};

export default function ForgotPasswordPage() {
  return (
    <AuthShell
      title="Reset your password"
      intro="Enter the email you signed up with and we'll send you a link."
      footer={
        <Link href="/login" className="underline underline-offset-2">
          Back to sign in
        </Link>
      }
    >
      <ForgotForm />
    </AuthShell>
  );
}
