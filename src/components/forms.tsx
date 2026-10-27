"use client";

import { useEffect, useId, useRef, useState, type ComponentProps, type ReactNode } from "react";
import { useFormStatus } from "react-dom";
import type { FormState } from "@/lib/forms";
import { PASSWORD_RULES } from "@/lib/password-policy";

export function FormAlert({ state }: { state: FormState }) {
  const ref = useRef<HTMLDivElement>(null);
  useEffect(() => {
    if (state.message && !state.ok) ref.current?.focus();
  }, [state]);
  if (!state.message) return null;
  return (
    <div
      ref={ref}
      tabIndex={-1}
      role={state.ok ? "status" : "alert"}
      className={`rounded-lg px-4 py-3 text-sm outline-none ${state.ok ? "bg-ok-soft text-ok" : "bg-err-soft text-err"}`}
    >
      {state.message}
    </div>
  );
}

type FieldProps = ComponentProps<"input"> & {
  label: string;
  name: string;
  errors?: string[];
  hint?: ReactNode;
  after?: ReactNode;
};

export function Field({ label, name, errors, hint, after, className = "", "aria-describedby": extraDescribedBy, ...input }: FieldProps) {
  const id = useId();
  const errorId = `${id}-error`;
  const hintId = `${id}-hint`;
  const invalid = !!errors?.length;
  return (
    <div className={className}>
      <label htmlFor={id} className="block text-sm font-medium">
        {label}
      </label>
      <input
        id={id}
        name={name}
        aria-invalid={invalid || undefined}
        aria-describedby={[invalid ? errorId : null, hint && !invalid ? hintId : null, extraDescribedBy].filter(Boolean).join(" ") || undefined}
        className={`mt-1.5 block h-11 w-full rounded-lg border bg-bg px-3 text-base placeholder:text-muted ${invalid ? "border-err" : "border-border"}`}
        {...input}
      />
      {after}
      {hint && !invalid && (
        <p id={hintId} className="mt-1.5 text-xs text-muted">
          {hint}
        </p>
      )}
      {invalid && (
        <p id={errorId} className="mt-1.5 text-sm text-err">
          {errors!.join(" ")}
        </p>
      )}
    </div>
  );
}

export function Checkbox({ name, label, errors, defaultChecked }: { name: string; label: ReactNode; errors?: string[]; defaultChecked?: boolean }) {
  const id = useId();
  const invalid = !!errors?.length;
  return (
    <div>
      <div className="flex items-start gap-3">
        <input
          id={id}
          name={name}
          type="checkbox"
          defaultChecked={defaultChecked}
          aria-invalid={invalid || undefined}
          aria-describedby={invalid ? `${id}-error` : undefined}
          className="mt-0.5 h-5 w-5 shrink-0 accent-[var(--accent)]"
        />
        <label htmlFor={id} className="text-sm">
          {label}
        </label>
      </div>
      {invalid && (
        <p id={`${id}-error`} className="mt-1 ml-8 text-sm text-err">
          {errors!.join(" ")}
        </p>
      )}
    </div>
  );
}

export function SubmitButton({ children, pendingText, variant = "primary", className = "" }: { children: ReactNode; pendingText: string; variant?: "primary" | "danger"; className?: string }) {
  const { pending } = useFormStatus();
  const colors = variant === "danger" ? "bg-err text-[var(--err-ink)]" : "bg-accent text-accent-ink";
  return (
    <button type="submit" disabled={pending} aria-disabled={pending} className={`inline-flex h-11 items-center justify-center rounded-lg px-5 font-medium hover:brightness-110 disabled:opacity-60 ${colors} ${className}`}>
      {pending ? pendingText : children}
    </button>
  );
}

// ---------- Password with live strength feedback ----------

type Strength = { score: 0 | 1 | 2 | 3 | 4; warning: string | null; suggestions: string[] };
let checker: Promise<(pw: string, inputs: string[]) => Strength> | null = null;

/** zxcvbn and its dictionaries are ~400 KB, so they load only when a password field is focused. */
function loadChecker() {
  return (checker ??= Promise.all([import("@zxcvbn-ts/core"), import("@zxcvbn-ts/language-common")]).then(([core, common]) => {
    const factory = new core.ZxcvbnFactory({ dictionary: { ...common.dictionary }, graphs: common.adjacencyGraphs });
    return (pw: string, inputs: string[]) => {
      const r = factory.check(pw, inputs);
      return { score: r.score, warning: r.feedback.warning, suggestions: r.feedback.suggestions };
    };
  }));
}

const LABELS = ["Very weak", "Weak", "Fair", "Strong", "Very strong"];
const COLORS = ["bg-err", "bg-err", "bg-accent", "bg-ok", "bg-ok"];

export function PasswordField({ name, label, errors, userInputs = [], autoComplete = "new-password" }: { name: string; label: string; errors?: string[]; userInputs?: string[]; autoComplete?: string }) {
  const [value, setValue] = useState("");
  const [show, setShow] = useState(false);
  const [result, setResult] = useState<Strength | null>(null);
  const strength = value ? result : null;
  const meterId = useId();

  useEffect(() => {
    if (!value) return;
    let cancelled = false;
    const t = setTimeout(() => {
      loadChecker().then((check) => {
        if (!cancelled) setResult(check(value, userInputs));
      });
    }, 120);
    return () => {
      cancelled = true;
      clearTimeout(t);
    };
    // userInputs is a fresh array each render; its contents only matter alongside value.
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [value]);

  return (
    <Field
      label={label}
      name={name}
      type={show ? "text" : "password"}
      autoComplete={autoComplete}
      required
      minLength={12}
      maxLength={128}
      value={value}
      onChange={(e) => setValue(e.target.value)}
      onFocus={() => void loadChecker()}
      errors={errors}
      aria-describedby={meterId}
      after={
        <div id={meterId} className="mt-2 space-y-2">
          <button type="button" onClick={() => setShow((s) => !s)} className="text-xs text-muted underline underline-offset-2 hover:text-text">
            {show ? "Hide password" : "Show password"}
          </button>
          <div className="flex items-center gap-3" aria-live="polite">
            <div className="flex h-1.5 flex-1 gap-1" aria-hidden="true">
              {[0, 1, 2, 3].map((i) => (
                <span key={i} className={`h-full flex-1 rounded-full ${strength && value && strength.score > i ? COLORS[strength.score] : "bg-surface-2"}`} />
              ))}
            </div>
            <span className="w-24 text-right text-xs text-muted">{value ? (strength ? LABELS[strength.score] : "Checking…") : "Strength"}</span>
          </div>
          {strength?.warning && <p className="text-xs text-err">{strength.warning}</p>}
          <ul className="grid grid-cols-1 gap-x-4 gap-y-1 text-xs sm:grid-cols-2">
            {PASSWORD_RULES.map((r) => {
              const ok = r.test(value);
              return (
                <li key={r.id} className={ok ? "text-ok" : "text-muted"}>
                  <span aria-hidden="true">{ok ? "✓" : "○"}</span> {r.label}
                  <span className="sr-only">{ok ? " (met)" : " (not met)"}</span>
                </li>
              );
            })}
          </ul>
          {value && strength && strength.score < 3 && strength.suggestions[0] && <p className="text-xs text-muted">{strength.suggestions[0]}</p>}
        </div>
      }
    />
  );
}

// ---------- Proof-of-work CAPTCHA ----------

async function sha256Hex(s: string) {
  const buf = await crypto.subtle.digest("SHA-256", new TextEncoder().encode(s));
  return Array.from(new Uint8Array(buf), (b) => b.toString(16).padStart(2, "0")).join("");
}

export function Captcha({ required }: { required: boolean }) {
  const [status, setStatus] = useState<"idle" | "working" | "done" | "error">("idle");
  const [solution, setSolution] = useState("");

  async function solve() {
    setStatus("working");
    try {
      const res = await fetch("/api/captcha", { cache: "no-store" });
      if (!res.ok) throw new Error();
      const c = (await res.json()) as { salt: string; challenge: string; max: number; signature: string };
      for (let n = 0; n <= c.max; n++) {
        if ((await sha256Hex(`${c.salt}${n}`)) === c.challenge) {
          setSolution(JSON.stringify({ ...c, number: n }));
          setStatus("done");
          return;
        }
      }
      throw new Error();
    } catch {
      setStatus("error");
    }
  }

  if (!required) return null;
  return (
    <fieldset className="rounded-lg border border-border p-4">
      <legend className="px-1 text-sm font-medium">Security check</legend>
      <input type="hidden" name="captcha" value={solution} />
      <div className="flex items-center gap-3 text-sm">
        {status === "done" ? (
          <p className="text-ok" role="status">
            ✓ Verified — you can continue.
          </p>
        ) : (
          <>
            <button
              type="button"
              onClick={solve}
              disabled={status === "working"}
              className="inline-flex h-9 items-center gap-2 rounded-lg border border-border px-3 hover:bg-surface-2 disabled:opacity-60"
            >
              {status === "working" ? "Verifying…" : "I'm not a robot"}
            </button>
            <p className="text-xs text-muted" aria-live="polite">
              {status === "error" ? "That didn't work — please try again." : "Your browser does a quick calculation. No tracking, no images to click."}
            </p>
          </>
        )}
      </div>
    </fieldset>
  );
}
