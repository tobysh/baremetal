import type { Metadata } from "next";
import { Suspense } from "react";
import { SettingsNav, SettingsSkeleton } from "@/components/settings/settings-nav";
import { TwoFactorManage, TwoFactorSetup } from "@/components/settings/settings-forms";
import { Badge, Card, Container } from "@/components/ui";
import { getUserDoc, requireUser } from "@/lib/auth";

export const metadata: Metadata = {
  title: "Security settings",
  description: "Protect your Baremetal account with two-factor authentication and backup codes.",
  robots: { index: false },
};

export default function SecurityPage({ searchParams }: PageProps<"/settings/security">) {
  return (
    <Container className="max-w-3xl py-10 sm:py-14">
      <h1 className="display text-3xl font-bold sm:text-4xl">Account settings</h1>
      <p className="mt-3 text-muted">Keep your account protected and your recovery options ready.</p>
      <SettingsNav current="security" />
      <Suspense fallback={<SettingsSkeleton />}>
        <SecurityBody searchParams={searchParams} />
      </Suspense>
    </Container>
  );
}

async function SecurityBody({ searchParams }: { searchParams: PageProps<"/settings/security">["searchParams"] }) {
  const user = await requireUser("/settings/security");
  const doc = await getUserDoc();
  const { required } = await searchParams;
  return (
    <div className="mt-8 space-y-8">
      {required === "admin" && !user.twoFactorEnabled && (
        <p role="alert" className="rounded-lg bg-err-soft px-4 py-3 text-sm text-err">
          Admin accounts must use two-factor authentication. Set it up below to access the admin area.
        </p>
      )}
      <Card as="section" className="p-6">
        <div className="flex flex-wrap items-center gap-3">
          <h2 className="display-sm text-lg font-bold">Two-factor authentication</h2>
          {user.twoFactorEnabled ? <Badge tone="ok">On</Badge> : <Badge>Off</Badge>}
        </div>
        <p className="mt-1 text-sm text-muted">
          Even if someone learns your password, they can&apos;t sign in without a code from your phone.
        </p>
        <div className="mt-5">
          {user.twoFactorEnabled ? (
            <TwoFactorManage backupCodesLeft={doc?.twoFactor?.backupCodeHashes.length ?? 0} isAdmin={user.role === "admin"} />
          ) : (
            <TwoFactorSetup />
          )}
        </div>
      </Card>
    </div>
  );
}
