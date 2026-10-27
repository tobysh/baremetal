import type { Metadata } from "next";
import Link from "next/link";
import { AuthShell } from "../auth-shell";
import { SignupForm } from "../auth-forms";

export const metadata: Metadata = {
  title: "Create your free account",
  description: "Create a free Baremetal account to save your progress through the courses and get help from the AI tutor.",
  alternates: { canonical: "/signup" },
};

export default function SignupPage() {
  return (
    <AuthShell
      title="Create your free account"
      intro="Save your progress and get help from the AI tutor. Takes a minute."
      footer={
        <>
          Already have an account?{" "}
          <Link href="/login" className="text-accent underline underline-offset-2">
            Sign in
          </Link>
        </>
      }
    >
      <SignupForm />
    </AuthShell>
  );
}
