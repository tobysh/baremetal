"use client";

import { useActionState, type ReactNode } from "react";
import { Field, FormAlert, SubmitButton } from "@/components/forms";
import { initialFormState, type FormState } from "@/lib/forms";
import { banAccount, createAccount, deleteAccountAsAdmin, liftSuspension, lockAccount, resendVerification, sendResetLink, signOutEverywhere } from "./actions";

type Act = (prev: FormState, fd: FormData) => Promise<FormState>;

function ActionForm({ action, userId, children, label, pending, variant = "primary" }: { action: Act; userId: string; children?: (s: FormState) => ReactNode; label: string; pending: string; variant?: "primary" | "danger" }) {
  const [state, run] = useActionState(action, initialFormState);
  return (
    <form action={run} className="space-y-3">
      <FormAlert state={state} />
      <input type="hidden" name="userId" value={userId} />
      {children?.(state)}
      <SubmitButton pendingText={pending} variant={variant}>{label}</SubmitButton>
    </form>
  );
}

const Reason = () => <Field label="Note (optional, kept in the audit log)" name="reason" maxLength={200} />;

export function UserActions({ userId, email, status, unverified }: { userId: string; email: string; status: "active" | "locked" | "banned"; unverified: boolean }) {
  return (
    <div className="space-y-6">
      <div className="grid gap-6 md:grid-cols-2">
        <section aria-labelledby="a-reset" className="space-y-2">
          <h3 id="a-reset" className="font-medium">Reset password</h3>
          <p className="text-sm text-muted">Emails a single-use link, valid 30 minutes. Nobody but the account owner ever sees a new password, and they are signed out everywhere once they use it.</p>
          <ActionForm action={sendResetLink} userId={userId} label="Email reset link" pending="Sending…" />
        </section>
        {unverified ? (
          <section aria-labelledby="a-verify" className="space-y-2">
            <h3 id="a-verify" className="font-medium">Resend verification</h3>
            <p className="text-sm text-muted">The account has not confirmed its email yet. A new link replaces the old one.</p>
            <ActionForm action={resendVerification} userId={userId} label="Resend verification email" pending="Sending…" />
          </section>
        ) : (
          <section aria-labelledby="a-out" className="space-y-2">
            <h3 id="a-out" className="font-medium">Sign out everywhere</h3>
            <p className="text-sm text-muted">Ends every session on every device. They can sign in again.</p>
            <ActionForm action={signOutEverywhere} userId={userId} label="Sign out all devices" pending="Working…" />
          </section>
        )}
      </div>

      {status === "active" ? (
        <section aria-labelledby="a-lock" className="space-y-2">
          <h3 id="a-lock" className="font-medium">Lock account</h3>
          <p className="text-sm text-muted">Blocks sign-in and ends all sessions. Reversible; a timed lock lifts itself.</p>
          <ActionForm action={lockAccount} userId={userId} label="Lock account" pending="Locking…">
            {(s) => (
              <>
                <div>
                  <label htmlFor="duration" className="block text-sm font-medium">Duration</label>
                  <select id="duration" name="duration" defaultValue="24h" className="mt-1.5 block h-11 w-full rounded-lg border border-field bg-bg px-3">
                    <option value="1h">1 hour</option>
                    <option value="24h">24 hours</option>
                    <option value="7d">7 days</option>
                    <option value="30d">30 days</option>
                    <option value="forever">Until I unlock it</option>
                  </select>
                  {s.errors?.duration && <p className="mt-1.5 text-sm text-err">{s.errors.duration.join(" ")}</p>}
                </div>
                <Reason />
              </>
            )}
          </ActionForm>
        </section>
      ) : (
        <section aria-labelledby="a-unlock" className="space-y-2">
          <h3 id="a-unlock" className="font-medium">{status === "banned" ? "Lift ban" : "Unlock account"}</h3>
          <ActionForm action={liftSuspension} userId={userId} label={status === "banned" ? "Lift ban" : "Unlock"} pending="Working…" />
        </section>
      )}

      <details className="rounded-lg border border-border p-4">
        <summary className="cursor-pointer font-medium text-err">Danger zone</summary>
        <div className="mt-4 grid gap-6 md:grid-cols-2">
          {status !== "banned" && (
            <section aria-labelledby="a-ban" className="space-y-2">
              <h3 id="a-ban" className="font-medium">Ban account</h3>
              <p className="text-sm text-muted">Permanent until lifted. Signs them out, hides the profile and leaderboard entry, cancels reset links. Progress is kept.</p>
              <ActionForm action={banAccount} userId={userId} label="Ban account" pending="Banning…" variant="danger">
                {(s) => (
                  <>
                    <Reason />
                    <Field label={`Type ${email} to confirm`} name="confirm" autoComplete="off" spellCheck={false} errors={s.errors?.confirm} />
                  </>
                )}
              </ActionForm>
            </section>
          )}
          <section aria-labelledby="a-del" className="space-y-2">
            <h3 id="a-del" className="font-medium">Delete account</h3>
            <p className="text-sm text-muted">Permanently removes the account, progress, XP, reviews and tutor history. This cannot be undone and the person is not notified.</p>
            <ActionForm action={deleteAccountAsAdmin} userId={userId} label="Delete permanently" pending="Deleting…" variant="danger">
              {(s) => (
                <>
                  <Reason />
                  <Field label={`Type ${email} to confirm`} name="confirm" autoComplete="off" spellCheck={false} errors={s.errors?.confirm} />
                </>
              )}
            </ActionForm>
          </section>
        </div>
      </details>
    </div>
  );
}

export function CreateAccountForm() {
  const [state, run] = useActionState(createAccount, initialFormState);
  const v = state.values ?? {};
  return (
    <form action={run} className="space-y-4" noValidate>
      <FormAlert state={state} />
      <div className="grid gap-4 md:grid-cols-3">
        <Field label="Name" name="name" required maxLength={80} defaultValue={v.name} errors={state.errors?.name} />
        <Field label="Username" name="username" required minLength={3} maxLength={20} autoCapitalize="none" spellCheck={false} defaultValue={v.username} errors={state.errors?.username} hint="3–20 lowercase letters, numbers, - or _." />
        <Field label="Email" name="email" type="email" required defaultValue={v.email} errors={state.errors?.email} />
      </div>
      <p className="text-sm text-muted">They get a verification email, confirm it and choose their own password. They accept the Terms at that point; until then the account cannot sign in.</p>
      <SubmitButton pendingText="Creating…">Create account and send invite</SubmitButton>
    </form>
  );
}
