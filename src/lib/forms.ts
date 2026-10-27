import type { z } from "zod";

/** State returned by server actions used with useActionState. */
export type FormState = {
  ok?: boolean;
  /** Form-level message (e.g. "Incorrect email or password"). */
  message?: string;
  /** Per-field errors, keyed by input name. */
  errors?: Record<string, string[] | undefined>;
  /** Non-secret values to put back into the form after an error. */
  values?: Record<string, string>;
  captchaRequired?: boolean;
  /** Extra data for the client (e.g. backup codes to show once). */
  data?: Record<string, unknown>;
};

export const initialFormState: FormState = {};

export function fieldErrors(error: z.ZodError): FormState["errors"] {
  const out: Record<string, string[]> = {};
  for (const issue of error.issues) {
    const key = String(issue.path[0] ?? "form");
    (out[key] ??= []).push(issue.message);
  }
  return out;
}

export function formValues(formData: FormData, keys: string[]): Record<string, string> {
  return Object.fromEntries(keys.map((k) => [k, String(formData.get(k) ?? "")]));
}
