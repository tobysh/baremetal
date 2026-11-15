import type { Health } from "./admin";

export type HealthItem = { label: string; ok: boolean; value: string; detail: string };

/** One row per health check. Shared by the overview panel and the nav alert. */
export function healthItems(h: Health): HealthItem[] {
  return [
    { label: "Database", ok: h.db.ok, value: h.db.ok ? `OK (${h.db.pingMs} ms)` : "Unreachable", detail: "MongoDB ping" },
    { label: "Email (SMTP)", ok: h.smtp.configured, value: h.smtp.configured ? "Configured" : "Not set up", detail: h.smtp.configured ? `${h.smtp.host} · from ${h.smtp.from ?? "default"}` : "Set SMTP_URL and EMAIL_FROM. Verification and reset emails are not delivered." },
    { label: "Email queue", ok: h.queue.retrying === 0 && (h.queue.oldestMinutes ?? 0) < 15, value: `${h.queue.pending} waiting`, detail: h.queue.retrying ? `${h.queue.retrying} failing and retrying. Oldest ${h.queue.oldestMinutes} min.` : h.queue.pending ? `Oldest ${h.queue.oldestMinutes} min` : "Empty" },
    { label: "Code runner", ok: h.runner.mode === "local" || h.runner.reachable === true, value: h.runner.mode === "socket" ? (h.runner.reachable ? "Service reachable" : "Service down") : "Local (dev)", detail: h.runner.mode === "socket" ? "RUNNER_SOCKET" : "No RUNNER_SOCKET. Production requires one." },
    { label: "AI tutor", ok: h.tutor.configured, value: h.tutor.configured ? `Ready (${h.tutor.provider})` : "No API key", detail: h.tutor.provider === "claude" ? "ANTHROPIC_API_KEY" : "Local Ollama model" },
    { label: "Auth secret", ok: h.secret.ok, value: h.secret.ok ? "Set" : "Too short or missing", detail: "AUTH_SECRET, 32+ characters" },
    { label: "Accounts", ok: h.accounts.adminsWithout2fa === 0, value: `${h.accounts.total} total`, detail: `${h.accounts.unverified} unverified · ${h.accounts.suspended} locked or banned${h.accounts.adminsWithout2fa ? ` · ${h.accounts.adminsWithout2fa} admin without 2FA` : ""}` },
  ];
}

export function failingChecks(h: Health): HealthItem[] {
  return healthItems(h).filter((i) => !i.ok);
}
