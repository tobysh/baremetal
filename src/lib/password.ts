import "server-only";
import { createHash } from "node:crypto";
import { readFileSync } from "node:fs";
import path from "node:path";
import { PASSWORD_MAX, basePassword, containsPersonalInfo, failedRules } from "./password-policy";

let common: Set<string> | null = null;
function commonPasswords(): Set<string> {
  if (!common) {
    const file = path.join(process.cwd(), "data", "common-passwords.txt");
    common = new Set(
      readFileSync(file, "utf8")
        .split("\n")
        .map((l) => l.trim().toLowerCase())
        .filter(Boolean),
    );
  }
  return common;
}

/**
 * Have I Been Pwned range API (k-anonymity): only the first 5 hex chars of the
 * SHA-1 leave the server. Fails open (returns 0) if the service is unreachable,
 * so an outage never blocks sign-up; the other checks still apply.
 */
async function pwnedCount(pw: string): Promise<number> {
  const hash = createHash("sha1").update(pw).digest("hex").toUpperCase();
  const prefix = hash.slice(0, 5);
  const suffix = hash.slice(5);
  try {
    const res = await fetch(`https://api.pwnedpasswords.com/range/${prefix}`, {
      headers: { "Add-Padding": "true", "User-Agent": "baremetal-password-check" },
      signal: AbortSignal.timeout(3000),
      cache: "no-store",
    });
    if (!res.ok) return 0;
    for (const line of (await res.text()).split("\n")) {
      const [s, count] = line.trim().split(":");
      if (s === suffix) return Number(count) || 0;
    }
  } catch {}
  return 0;
}

/** Returns an error message, or null when the password is acceptable. */
export async function checkPassword(pw: string, personal: string[] = []): Promise<string | null> {
  if (pw.length > PASSWORD_MAX) return `Passwords can be at most ${PASSWORD_MAX} characters.`;
  const missing = failedRules(pw);
  if (missing.length) return `Your password needs: ${missing.map((r) => r.label.toLowerCase()).join(", ")}.`;
  const list = commonPasswords();
  if (list.has(pw.toLowerCase()) || list.has(basePassword(pw)))
    return "That password is too common — it's on lists attackers try first. Try a passphrase of a few unrelated words.";
  if (containsPersonalInfo(pw, personal)) return "Don't use your name or email address in your password.";
  if (/(.)\1{4,}/.test(pw)) return "Avoid long runs of the same character.";
  const pwned = await pwnedCount(pw);
  if (pwned > 0)
    return `This password has appeared in ${pwned.toLocaleString("en-GB")} data breaches, so attackers already know it. Please choose another.`;
  return null;
}
