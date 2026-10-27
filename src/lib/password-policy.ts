// Shared by the browser (live feedback) and the server (enforcement).
// The server is the source of truth: it also checks the common-password list
// and Have I Been Pwned, which the browser does not.

export const PASSWORD_MIN = 12;
export const PASSWORD_MAX = 128;

export type PolicyRule = { id: string; label: string; test: (pw: string) => boolean };

export const PASSWORD_RULES: PolicyRule[] = [
  { id: "length", label: `At least ${PASSWORD_MIN} characters`, test: (pw) => pw.length >= PASSWORD_MIN },
  { id: "lower", label: "A lowercase letter", test: (pw) => /\p{Ll}/u.test(pw) },
  { id: "upper", label: "An uppercase letter", test: (pw) => /\p{Lu}/u.test(pw) },
  { id: "number", label: "A number", test: (pw) => /\d/.test(pw) },
  { id: "symbol", label: "A symbol (e.g. ! ? # %)", test: (pw) => /[^\p{L}\d\s]/u.test(pw) || /\s/.test(pw.trim()) },
];

export function failedRules(pw: string): PolicyRule[] {
  return PASSWORD_RULES.filter((r) => !r.test(pw));
}

/** Reject passwords built from the user's own details, e.g. "Toby2026!toby". */
export function containsPersonalInfo(pw: string, personal: string[]): boolean {
  const lower = pw.toLowerCase();
  return personal
    .flatMap((p) => p.toLowerCase().split(/[^a-z0-9]+/))
    .filter((part) => part.length >= 4)
    .some((part) => lower.includes(part));
}

/** Strip the decoration people add to weak passwords: "Password123!" -> "password". */
export function basePassword(pw: string): string {
  return pw
    .toLowerCase()
    .replace(/[^a-z]+$/g, "")
    .replace(/^[^a-z]+/g, "")
    .replace(/0/g, "o")
    .replace(/1/g, "i")
    .replace(/3/g, "e")
    .replace(/4/g, "a")
    .replace(/5/g, "s")
    .replace(/@/g, "a")
    .replace(/\$/g, "s");
}
