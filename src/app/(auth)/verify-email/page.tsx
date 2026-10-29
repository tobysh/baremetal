import type { Metadata } from "next";
import { AuthShell } from "../auth-shell";
import { VerifyEmailForm } from "../fragment-forms";

export const metadata: Metadata = { title: "Verify your email", description: "Verify your Baremetal email address.", robots: { index: false }, referrer: "no-referrer" };

export default function VerifyEmailPage() {
  return <AuthShell title="Verify your email" intro="Confirm your email address to finish creating your account."><VerifyEmailForm /></AuthShell>;
}
