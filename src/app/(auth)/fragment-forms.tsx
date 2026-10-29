"use client";

import Link from "next/link";
import { useActionState, useSyncExternalStore } from "react";
import { Field, FormAlert, PasswordField, SubmitButton } from "@/components/forms";
import { initialFormState } from "@/lib/forms";
import { ResetForm } from "./auth-forms";
import { verifyEmail } from "./actions";

const subscribe = () => () => {};
const getFragment = () => window.location.hash.slice(1);
const serverFragment = () => "";

export function FragmentResetForm({ legacyToken }: { legacyToken?: string }) {
  const fragment = useSyncExternalStore(subscribe, getFragment, serverFragment);
  const token = fragment || legacyToken || "";
  if (token.length < 20) return <p className="text-sm">Open your reset link, or <Link className="text-accent underline" href="/forgot-password">request a new one</Link>.</p>;
  return <ResetForm token={token} />;
}

export function VerifyEmailForm() {
  const token = useSyncExternalStore(subscribe, getFragment, serverFragment);
  const [state, action] = useActionState(verifyEmail, initialFormState);
  return <form action={action} className="space-y-4">
    <FormAlert state={state} />
    <input type="hidden" name="token" value={token} />
    {!state.ok && token.length >= 20 && <>
      <p className="text-sm">Choose your password to finish creating your account.</p>
      <PasswordField name="password" label="Password" errors={state.errors?.password} />
      <Field name="confirm" label="Confirm password" type="password" autoComplete="new-password" required errors={state.errors?.confirm} />
      <SubmitButton pendingText="Verifying…">Verify email and set password</SubmitButton>
    </>}
    {state.ok && <Link href="/login" className="text-accent underline">Sign in</Link>}
  </form>;
}
