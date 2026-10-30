/** Settings that have unsafe or placeholder fallbacks and so must be explicit in production. */
export function productionConfigProblems(env: NodeJS.ProcessEnv): string[] {
  const problems: string[] = [];
  if (!env.NEXT_PUBLIC_SITE_URL?.startsWith("https://")) problems.push("NEXT_PUBLIC_SITE_URL must be set to the public https:// URL (it is inlined at build time, and emailed links use it)");
  for (const name of ["LEGAL_ENTITY_NAME", "LEGAL_ADDRESS", "LEGAL_CONTACT_EMAIL", "SMTP_URL", "EMAIL_FROM"]) {
    if (!env[name]?.trim()) problems.push(`${name} is not set`);
  }
  if ((env.AUTH_SECRET ?? "").length < 32) problems.push("AUTH_SECRET must be at least 32 characters");
  return problems;
}

export async function register() {
  if (process.env.NEXT_RUNTIME !== "nodejs") return;
  const hops = process.env.TRUSTED_PROXY_HOPS;
  if ((process.env.NODE_ENV === "production" || hops !== undefined) && (!hops || !/^[1-9][0-9]*$/.test(hops) || !Number.isSafeInteger(Number(hops)))) {
    throw new Error(
      "TRUSTED_PROXY_HOPS is not set. Per-IP rate limits trust X-Forwarded-For, so production needs the number of reverse proxies in front of the app (see docs/deployment-hardening.md).",
    );
  }
  if (process.env.NODE_ENV === "production") {
    const missing = productionConfigProblems(process.env);
    if (missing.length) {
      throw new Error(`Production configuration is incomplete:\n - ${missing.join("\n - ")}\nSee docs/deployment-hardening.md.`);
    }
  }
  const { assertRunnerConfiguration } = await import("./lib/runner-client");
  assertRunnerConfiguration();
  if (!process.env.RUNNER_SOCKET) {
    const { cleanupStaleWorkdirs } = await import("./lib/runner-engine");
    await cleanupStaleWorkdirs();
  }
  console.log(`[boot] TRUSTED_PROXY_HOPS=${process.env.TRUSTED_PROXY_HOPS ?? "(unset, dev)"}`);
}
