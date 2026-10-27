"use client";

import Link from "next/link";
import { useActionState } from "react";
import { Captcha, Checkbox, Field, FormAlert, PasswordField, SubmitButton } from "@/components/forms";
import { initialFormState } from "@/lib/forms";
import { EXPERIENCE_LEVELS } from "@/lib/models";
import { login, requestPasswordReset, resetPassword, signup, verifySecondFactor } from "./actions";

export function LoginForm({ next }: { next: string }) {
  const [state, action] = useActionState(login, initialFormState);
  return (
    <form action={action} className="space-y-5" noValidate>
      <FormAlert state={state} />
      <input type="hidden" name="next" value={next} />
      <Field label="Email" name="email" type="email" autoComplete="email" inputMode="email" required defaultValue={state.values?.email} errors={state.errors?.email} />
      <Field
        label="Password"
        name="password"
        type="password"
        autoComplete="current-password"
        required
        errors={state.errors?.password}
        after={
          <p className="mt-1.5 text-right text-sm">
            <Link href="/forgot-password" className="text-muted underline underline-offset-2 hover:text-text">
              Forgot your password?
            </Link>
          </p>
        }
      />
      {/* key resets the widget so each attempt needs a fresh solution */}
      <Captcha key={JSON.stringify(state)} required={!!state.captchaRequired} />
      <SubmitButton pendingText="Signing in…" className="w-full">
        Sign in
      </SubmitButton>
    </form>
  );
}

export function SignupForm() {
  const [state, action] = useActionState(signup, initialFormState);
  const v = state.values ?? {};
  return (
    <form action={action} className="space-y-5" noValidate>
      <FormAlert state={state} />
      <Field label="What should we call you?" name="name" autoComplete="given-name" required maxLength={80} defaultValue={v.name} errors={state.errors?.name} />
      <Field
        label="Username"
        name="username"
        autoComplete="username"
        autoCapitalize="none"
        spellCheck={false}
        required
        minLength={3}
        maxLength={20}
        pattern="[a-zA-Z0-9_\-]+"
        defaultValue={v.username}
        errors={state.errors?.username}
        hint="Shown publicly on the leaderboard and your profile, with your level and completed lessons. You can hide your profile in settings."
      />
      <Field label="Email" name="email" type="email" autoComplete="email" inputMode="email" required defaultValue={v.email} errors={state.errors?.email} />
      <PasswordField name="password" label="Password" errors={state.errors?.password} userInputs={[v.name ?? "", v.email ?? "", v.username ?? ""]} />
      <Field label="Confirm password" name="confirm" type="password" autoComplete="new-password" required errors={state.errors?.confirm} />
      <fieldset aria-describedby={state.errors?.experience ? "experience-error" : undefined}>
        <legend className="text-sm font-medium">How much programming have you done?</legend>
        <p className="mt-1 text-xs text-muted">The tutor uses this to pitch explanations at the right level. You can change it later.</p>
        <div className="mt-2 grid gap-2">
          {Object.entries(EXPERIENCE_LEVELS).map(([value, label]) => (
            <label key={value} className="flex cursor-pointer items-center gap-3 rounded-lg border border-border px-3 py-2.5 text-sm has-[:checked]:border-accent has-[:checked]:bg-accent-soft">
              <input type="radio" name="experience" value={value} defaultChecked={v.experience ? v.experience === value : value === "none"} className="accent-[var(--accent)]" />
              {label}
            </label>
          ))}
        </div>
        {state.errors?.experience && (
          <p id="experience-error" className="mt-1.5 text-sm text-err">
            {state.errors.experience.join(" ")}
          </p>
        )}
      </fieldset>
      <Checkbox
        name="terms"
        errors={state.errors?.terms}
        label={
          <>
            I agree to the{" "}
            <Link href="/terms" target="_blank" className="text-accent underline underline-offset-2">
              Terms of Service
            </Link>{" "}
            and have read the{" "}
            <Link href="/privacy" target="_blank" className="text-accent underline underline-offset-2">
              Privacy Notice
            </Link>
            .
          </>
        }
      />
      <Checkbox name="age" errors={state.errors?.age} label="I am 16 or older." />
      <Captcha key={JSON.stringify(state)} required />
      <SubmitButton pendingText="Creating your account…" className="w-full">
        Create account
      </SubmitButton>
    </form>
  );
}

export function VerifyForm() {
  const [state, action] = useActionState(verifySecondFactor, initialFormState);
  return (
    <form action={action} className="space-y-5" noValidate>
      <FormAlert state={state} />
      <Field
        label="Authentication code"
        name="code"
        autoComplete="one-time-code"
        inputMode="text"
        autoFocus
        required
        maxLength={12}
        errors={state.errors?.code}
        hint="Enter the 6-digit code from your authenticator app, or one of your backup codes."
      />
      <SubmitButton pendingText="Checking…" className="w-full">
        Verify
      </SubmitButton>
    </form>
  );
}

export function ForgotForm() {
  const [state, action] = useActionState(requestPasswordReset, initialFormState);
  if (state.ok) return <FormAlert state={state} />;
  return (
    <form action={action} className="space-y-5" noValidate>
      <FormAlert state={state} />
      <Field label="Email" name="email" type="email" autoComplete="email" inputMode="email" required defaultValue={state.values?.email} errors={state.errors?.email} />
      <SubmitButton pendingText="Sending…" className="w-full">
        Send reset link
      </SubmitButton>
    </form>
  );
}

export function ResetForm({ token }: { token: string }) {
  const [state, action] = useActionState(resetPassword, initialFormState);
  return (
    <form action={action} className="space-y-5" noValidate>
      <FormAlert state={state} />
      <input type="hidden" name="token" value={token} />
      <PasswordField name="password" label="New password" errors={state.errors?.password} />
      <Field label="Confirm new password" name="confirm" type="password" autoComplete="new-password" required errors={state.errors?.confirm} />
      <SubmitButton pendingText="Saving…" className="w-full">
        Set new password
      </SubmitButton>
    </form>
  );
}
