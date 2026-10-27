import type { Metadata } from "next";
import Link from "next/link";
import { Suspense } from "react";
import { CookieSettingsButton } from "@/components/consent";
import { PasswordForm, PrivacyControls, ProfileForm } from "@/components/settings/settings-forms";
import { SettingsNav, SettingsSkeleton } from "@/components/settings/settings-nav";
import { Card, Container } from "@/components/ui";
import { requireUser } from "@/lib/auth";

export const metadata: Metadata = {
  title: "Account settings",
  description: "Manage your Baremetal profile, password, privacy and data.",
  robots: { index: false },
};

export default function SettingsPage() {
  return (
    <Container className="max-w-3xl py-12">
      <h1 className="text-3xl font-bold tracking-tight">Account settings</h1>
      <SettingsNav current="account" />
      <Suspense fallback={<SettingsSkeleton />}>
        <SettingsBody />
      </Suspense>
    </Container>
  );
}

async function SettingsBody() {
  const user = await requireUser("/settings");
  return (
    <div className="mt-8 space-y-8">
      <Card as="section" className="p-6">
        <h2 className="text-lg font-semibold">Profile</h2>
        <p className="mt-1 text-sm text-muted">Signed in as {user.email}</p>
        <div className="mt-5">
          <ProfileForm name={user.name} username={user.username} experience={user.experience} profilePublic={user.profilePublic} />
        </div>
      </Card>
      <Card as="section" className="p-6">
        <h2 className="text-lg font-semibold">Password</h2>
        <p className="mt-1 text-sm text-muted">Changing your password signs you out on every other device.</p>
        <div className="mt-5">
          <PasswordForm email={user.email} name={user.name} />
        </div>
      </Card>
      <Card as="section" className="p-6">
        <h2 className="text-lg font-semibold">Cookies</h2>
        <p className="mt-1 text-sm text-muted">Choose whether we can count your visits with our privacy-friendly analytics.</p>
        <div className="mt-3 text-sm text-accent underline underline-offset-2">
          <CookieSettingsButton />
        </div>
      </Card>
      <Card as="section" className="p-6">
        <h2 className="text-lg font-semibold">Your data</h2>
        <p className="mt-1 text-sm text-muted">
          Your rights are explained in the{" "}
          <Link href="/privacy" className="text-accent underline underline-offset-2">
            Privacy Notice
          </Link>
          .
        </p>
        <div className="mt-5">
          <PrivacyControls twoFactor={user.twoFactorEnabled} />
        </div>
      </Card>
    </div>
  );
}
