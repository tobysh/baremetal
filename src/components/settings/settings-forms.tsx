"use client";

import { useActionState, useState, useTransition } from "react";
import { Checkbox, Field, FormAlert, PasswordField, SubmitButton } from "@/components/forms";
import { initialFormState, type FormState } from "@/lib/forms";
import { EXPERIENCE_LEVELS, type ExperienceLevel } from "@/lib/models";
import {
  beginTwoFactorSetup,
  changePassword,
  clearTutorHistory,
  confirmTwoFactor,
  deleteAccount,
  disableTwoFactor,
  regenerateBackupCodes,
  updateProfile,
} from "@/app/settings/actions";

export function ProfileForm({
  name,
  username,
  experience,
  profilePublic,
}: {
  name: string;
  username: string | null;
  experience: ExperienceLevel;
  profilePublic: boolean;
}) {
  const [state, action] = useActionState(updateProfile, initialFormState);
  const v = state.values ?? { name, username: username ?? "", experience, profilePublic: profilePublic ? "on" : "" };
  return (
    <form action={action} className="space-y-5" noValidate>
      <FormAlert state={state} />
      <Field label="Name" name="name" autoComplete="given-name" required maxLength={80} defaultValue={v.name} errors={state.errors?.name} hint="Private — only you see this." />
      <Field
        label="Username"
        name="username"
        autoComplete="username"
        autoCapitalize="none"
        spellCheck={false}
        required
        minLength={3}
        maxLength={20}
        defaultValue={v.username}
        errors={state.errors?.username}
        hint="Public. 3–20 lowercase letters, numbers, - or _."
      />
      <div>
        <label htmlFor="experience" className="block text-sm font-medium">
          Programming experience
        </label>
        <select id="experience" name="experience" defaultValue={v.experience} className="mt-1.5 block h-11 w-full rounded-lg border border-field bg-bg px-3">
          {Object.entries(EXPERIENCE_LEVELS).map(([value, label]) => (
            <option key={value} value={value}>
              {label}
            </option>
          ))}
        </select>
        <p className="mt-1.5 text-xs text-muted">The tutor adapts its explanations to this.</p>
      </div>
      <Checkbox
        name="profilePublic"
        defaultChecked={v.profilePublic === "on"}
        label={
          <>
            <span className="font-medium">Public profile</span>
            <span className="block text-muted">Show my username, level and completed lessons on the leaderboard and at my profile page.</span>
          </>
        }
      />
      <SubmitButton pendingText="Saving…">Save profile</SubmitButton>
    </form>
  );
}

export function PasswordForm({ email, name }: { email: string; name: string }) {
  const [state, action] = useActionState(changePassword, initialFormState);
  return (
    <form action={action} className="space-y-5" noValidate key={state.ok ? "done" : "form"}>
      <FormAlert state={state} />
      <Field label="Current password" name="current" type="password" autoComplete="current-password" required errors={state.errors?.current} />
      <PasswordField name="password" label="New password" errors={state.errors?.password} userInputs={[email, name]} />
      <Field label="Confirm new password" name="confirm" type="password" autoComplete="new-password" required errors={state.errors?.confirm} />
      <SubmitButton pendingText="Changing…">Change password</SubmitButton>
    </form>
  );
}

function BackupCodes({ codes }: { codes: string[] }) {
  const [copied, setCopied] = useState(false);
  return (
    <div className="rounded-lg border border-border bg-bg p-4">
      <p className="text-sm font-medium">Your backup codes</p>
      <p className="mt-1 text-sm text-muted">
        Save these somewhere safe, like a password manager. Each works once if you lose your phone. <strong className="text-text">You won&apos;t see them again.</strong>
      </p>
      <ul className="mt-3 grid grid-cols-2 gap-2 font-mono text-sm">
        {codes.map((c) => (
          <li key={c} className="rounded-md bg-surface-2 px-2 py-1 text-center">
            {c}
          </li>
        ))}
      </ul>
      <button
        type="button"
        className="mt-3 h-9 rounded-lg border border-border px-3 text-sm hover:bg-surface-2"
        onClick={() => navigator.clipboard.writeText(codes.join("\n")).then(() => setCopied(true))}
      >
        {copied ? "Copied" : "Copy codes"}
      </button>
    </div>
  );
}

export function TwoFactorSetup() {
  const [setup, setSetup] = useState<FormState | null>(null);
  const [pending, start] = useTransition();
  const [state, action] = useActionState(confirmTwoFactor, initialFormState);

  if (state.ok && state.data?.backupCodes) {
    return (
      <div className="space-y-4">
        <FormAlert state={state} />
        <BackupCodes codes={state.data.backupCodes as string[]} />
        <a href="/settings/security" className="inline-flex h-10 items-center rounded-lg bg-accent px-4 text-sm font-medium text-accent-ink">
          I&apos;ve saved my codes
        </a>
      </div>
    );
  }

  if (!setup?.data) {
    return (
      <div className="space-y-3">
        {setup?.message && <FormAlert state={setup} />}
        <button
          type="button"
          disabled={pending}
          onClick={() => start(async () => setSetup(await beginTwoFactorSetup()))}
          className="inline-flex h-10 items-center rounded-lg bg-accent px-4 text-sm font-medium text-accent-ink disabled:opacity-60"
        >
          {pending ? "Preparing…" : "Set up two-factor authentication"}
        </button>
      </div>
    );
  }

  return (
    <div className="space-y-5">
      <ol className="list-decimal space-y-2 pl-5 text-sm">
        <li>Install an authenticator app such as 1Password, Google Authenticator, Authy or Microsoft Authenticator.</li>
        <li>Scan this QR code with the app (or enter the key below by hand).</li>
        <li>Enter the 6-digit code the app shows.</li>
      </ol>
      <div className="flex flex-col items-start gap-4 sm:flex-row">
        {/* QR SVG is generated on our server by the qrcode library from a fixed otpauth:// URL. */}
        <div className="w-44 rounded-lg bg-white p-2" role="img" aria-label="QR code for your authenticator app" dangerouslySetInnerHTML={{ __html: setup.data.qrSvg as string }} />
        <div className="text-sm">
          <p className="text-muted">Setup key</p>
          <p className="mt-1 font-mono break-all select-all">{setup.data.secret as string}</p>
        </div>
      </div>
      <form action={action} className="space-y-4" noValidate>
        <FormAlert state={state} />
        <Field label="6-digit code" name="code" autoComplete="one-time-code" inputMode="numeric" pattern="[0-9 ]*" maxLength={7} required errors={state.errors?.code} className="max-w-xs" />
        <SubmitButton pendingText="Verifying…">Turn on</SubmitButton>
      </form>
    </div>
  );
}

export function TwoFactorManage({ backupCodesLeft, isAdmin }: { backupCodesLeft: number; isAdmin: boolean }) {
  const [regen, regenAction] = useActionState(regenerateBackupCodes, initialFormState);
  const [disable, disableAction] = useActionState(disableTwoFactor, initialFormState);
  return (
    <div className="space-y-8">
      <div>
        <h3 className="font-medium">Backup codes</h3>
        <p className="mt-1 text-sm text-muted">
          You have {backupCodesLeft} unused backup code{backupCodesLeft === 1 ? "" : "s"}.{backupCodesLeft <= 3 && " Consider generating new ones."}
        </p>
        {regen.ok && regen.data?.backupCodes ? (
          <div className="mt-3 space-y-3">
            <FormAlert state={regen} />
            <BackupCodes codes={regen.data.backupCodes as string[]} />
          </div>
        ) : (
          <form action={regenAction} className="mt-3 flex flex-wrap items-end gap-3" noValidate>
            <Field label="Current authenticator code" name="code" autoComplete="one-time-code" inputMode="numeric" maxLength={12} required errors={regen.errors?.code} className="w-56" />
            <SubmitButton pendingText="Generating…">Generate new codes</SubmitButton>
            <div className="w-full">
              <FormAlert state={regen} />
            </div>
          </form>
        )}
      </div>
      <div>
        <h3 className="font-medium">Turn off two-factor authentication</h3>
        {isAdmin ? (
          <p className="mt-1 text-sm text-muted">Admin accounts must keep two-factor authentication on.</p>
        ) : disable.ok ? (
          <FormAlert state={disable} />
        ) : (
          <form action={disableAction} className="mt-3 space-y-4" noValidate>
            <FormAlert state={disable} />
            <div className="grid gap-4 sm:grid-cols-2">
              <Field label="Password" name="password" type="password" autoComplete="current-password" required errors={disable.errors?.password} />
              <Field label="Authenticator or backup code" name="code" autoComplete="one-time-code" maxLength={12} required errors={disable.errors?.code} />
            </div>
            <SubmitButton pendingText="Turning off…" variant="danger">
              Turn off 2FA
            </SubmitButton>
          </form>
        )}
      </div>
    </div>
  );
}

export function PrivacyControls({ twoFactor }: { twoFactor: boolean }) {
  const [cleared, setCleared] = useState<FormState | null>(null);
  const [pending, start] = useTransition();
  const [state, action] = useActionState(deleteAccount, initialFormState);
  return (
    <div className="space-y-10">
      <div>
        <h3 className="font-medium">Download your data</h3>
        <p className="mt-1 text-sm text-muted">Everything we store about you — profile, progress, code, tutor conversations and security log — as a JSON file.</p>
        <a href="/api/account/export" className="mt-3 inline-flex h-10 items-center rounded-lg border border-border px-4 text-sm font-medium hover:bg-surface-2" download>
          Download my data
        </a>
      </div>
      <div>
        <h3 className="font-medium">Tutor conversations</h3>
        <p className="mt-1 text-sm text-muted">Delete every conversation you&apos;ve had with the AI tutor.</p>
        {cleared ? (
          <div className="mt-3">
            <FormAlert state={cleared} />
          </div>
        ) : (
          <button
            type="button"
            disabled={pending}
            onClick={() => start(async () => setCleared(await clearTutorHistory()))}
            className="mt-3 inline-flex h-10 items-center rounded-lg border border-border px-4 text-sm font-medium hover:bg-surface-2 disabled:opacity-60"
          >
            {pending ? "Deleting…" : "Delete tutor history"}
          </button>
        )}
      </div>
      <div className="rounded-xl border border-err/40 p-5">
        <h3 className="font-medium text-err">Delete your account</h3>
        <p className="mt-1 text-sm text-muted">
          This permanently deletes your account, progress, saved code and tutor conversations. It can&apos;t be undone. Security logs are kept for up to 90 days, as described in
          the Privacy Notice.
        </p>
        <form action={action} className="mt-4 space-y-4" noValidate>
          <FormAlert state={state} />
          <Field label='Type "delete my account" to confirm' name="confirm" autoComplete="off" required errors={state.errors?.confirm} />
          <Field label="Password" name="password" type="password" autoComplete="current-password" required errors={state.errors?.password} />
          {twoFactor && <Field label="Authenticator or backup code" name="code" autoComplete="one-time-code" maxLength={12} required errors={state.errors?.code} />}
          <SubmitButton pendingText="Deleting…" variant="danger">
            Permanently delete account
          </SubmitButton>
        </form>
      </div>
    </div>
  );
}
