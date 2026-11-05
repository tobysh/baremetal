import type { Metadata } from "next";
import Link from "next/link";
import { Suspense } from "react";
import { connection } from "next/server";
import { Card, Container, Skeleton } from "@/components/ui";
import { getSystemHealth, getTutorCost, type Health } from "@/lib/admin";
import { requireAdmin } from "@/lib/auth";
import { getPopularTutorAnswers } from "@/lib/tutor-popular";
import { getSiteStats } from "@/lib/stats";
import { purgeTutorAnswer } from "./actions";
import { first, usd } from "./admin-ui";

export const metadata: Metadata = {
  title: "Admin: analytics",
  description: "Site analytics and security overview.",
  robots: { index: false, follow: false },
};

const RANGES = [7, 30, 90, 180];

export default function AdminPage({ searchParams }: PageProps<"/admin">) {
  return (
    <Container className="py-12">
      <h1 className="display text-3xl font-bold">Admin overview</h1>
      <Suspense fallback={<AdminSkeleton />}>
        <AdminBody searchParams={searchParams} />
      </Suspense>
    </Container>
  );
}

async function AdminBody({ searchParams }: { searchParams: PageProps<"/admin">["searchParams"] }) {
  await connection();
  // Authorization happens here, on the server: role must be admin AND 2FA must be on.
  await requireAdmin();
  const requested = Number(first((await searchParams).days));
  const days = RANGES.includes(requested) ? requested : 30;
  const [s, popular, health, cost] = await Promise.all([getSiteStats(days), getPopularTutorAnswers(20), getSystemHealth(), getTutorCost(days)]);
  const maxDaily = Math.max(1, ...s.daily.map((d) => d.views));
  return (
    <div className="mt-4 space-y-6">
      <div className="flex flex-wrap items-center gap-3">
        <p className="text-muted">Last {days} days · consented visitors only · no raw IPs stored</p>
        <nav aria-label="Date range" className="flex gap-1.5">
          {RANGES.map((r) => (
            <Link key={r} href={r === 30 ? "/admin" : `/admin?days=${r}`} aria-current={r === days ? "true" : undefined} className={`rounded-lg border px-2.5 py-1 text-sm ${r === days ? "border-accent bg-accent-soft text-accent" : "border-border text-muted hover:text-text"}`}>
              {r}d
            </Link>
          ))}
        </nav>
        <a href={`/api/admin/stats?days=${days}&format=csv`} className="text-sm text-accent underline underline-offset-2">Download daily CSV</a>
      </div>

      <HealthPanel h={health} />

      <div className="grid gap-4 sm:grid-cols-2 lg:grid-cols-4">
        {[
          ["Pageviews", s.pageviews],
          ["Unique visitors (daily)", s.visitors],
          ["Sign-ups", s.funnel.signups],
          ["Lessons passed", s.funnel.testPasses],
        ].map(([label, value]) => (
          <Card key={label} className="p-5">
            <p className="text-sm text-muted">{label}</p>
            <p className="mt-1 text-3xl font-semibold tabular-nums">{Number(value).toLocaleString("en-GB")}</p>
          </Card>
        ))}
      </div>

      <Card as="section" className="p-6">
        <h2 className="display-sm font-semibold">Popular tutor answers</h2>
        <p className="mt-2 text-sm text-muted">Shared answers served at least five times. Frequently used entries stay cached for 90 days.</p>
        {popular.length === 0 ? <p className="mt-3 text-sm text-muted">No popular answers yet.</p> : (
          <ul className="mt-4 space-y-3">
            {popular.map((answer, i) => (
              <li key={`${answer.exerciseId}:${answer.kind}:${i}`} className="rounded border border-border p-3">
                <p className="text-sm font-medium">{answer.exerciseId} · {answer.kind} · {answer.hits} hits</p>
                {answer.question && <p className="mt-1 text-sm">{answer.question}</p>}
                <details className="mt-2 text-sm"><summary className="cursor-pointer text-muted">Show cached reply</summary><p className="mt-2 whitespace-pre-wrap break-words">{answer.reply}</p></details>
                <form action={purgeTutorAnswer} className="mt-2"><input type="hidden" name="id" value={answer.id} /><button type="submit" className="text-sm text-err underline underline-offset-2">Remove from cache</button></form>
              </li>
            ))}
          </ul>
        )}
      </Card>

      <Card as="section" className="p-6">
        <h2 className="display-sm font-semibold">Pageviews per day</h2>
        {s.daily.length === 0 ? (
          <p className="mt-3 text-sm text-muted">No data yet.</p>
        ) : (
          <div className="mt-4 flex h-40 items-end gap-1" role="img" aria-label={`Daily pageviews, ${s.daily.length} days`}>
            {s.daily.map((d) => (
              <div key={d.day} className="flex-1 rounded-t bg-accent" style={{ height: `${(d.views / maxDaily) * 100}%` }} title={`${d.day}: ${d.views} views, ${d.visitors} visitors`} />
            ))}
          </div>
        )}
      </Card>

      <div className="grid gap-6 lg:grid-cols-2">
        <StatTable title="Top pages" head={["Path", "Views"]} rows={s.topPages.map((p) => [p.path, p.views])} />
        <StatTable title="Top referrers" head={["Site", "Visits"]} rows={s.topReferrers.map((r) => [r.ref, r.visits])} />
        <StatTable title="Devices" head={["Device", "Views"]} rows={s.devices.map((d) => [d.device, d.count])} />
        <StatTable
          title={`AI tutor (${days} days)`}
          head={["Metric", "Value"]}
          rows={[
            ["Questions answered", s.tutor.requests],
            ["Served from response cache (no model call)", s.tutor.responseCacheHits],
            ["Input tokens (uncached)", s.tutor.inputTokens],
            ["Input tokens read from prompt cache", s.tutor.cacheReadTokens],
            ["Input tokens written to prompt cache", s.tutor.cacheWriteTokens],
            ["Output tokens", s.tutor.outputTokens],
          ]}
        />
        <StatTable
          title={`Tutor cost estimate (${days} days)`}
          head={["Metric", "Value"]}
          rows={[
            ["Estimated spend", usd(cost.totalUsd)],
            ["Average per day", usd(cost.perDay.length ? cost.totalUsd / cost.perDay.length : 0)],
            ["Average per answered question", usd(cost.requests ? cost.totalUsd / cost.requests : 0)],
            ["Served from cache", `${cost.requests ? Math.round((cost.cacheHits / cost.requests) * 100) : 0}%`],
            [`Users who hit the daily allowance (${usd(cost.ceilings.userDayUsd)})`, cost.userCeilingHits],
            [`Days the site ceiling was hit (${usd(cost.ceilings.globalDayUsd)})`, cost.globalCeilingDays],
          ]}
        />
        <StatTable
          title={`Security (${days} days)`}
          head={["Event", "Count"]}
          rows={[
            ["Failed sign-ins", s.security.failedLogins],
            ["Locked-out attempts", s.security.lockouts],
            ["Rate-limited requests", s.security.rateLimited],
          ]}
        />
      </div>
    </div>
  );
}

function HealthPanel({ h }: { h: Health }) {
  const items: { label: string; ok: boolean; value: string; detail: string }[] = [
    { label: "Database", ok: h.db.ok, value: h.db.ok ? `OK (${h.db.pingMs} ms)` : "Unreachable", detail: "MongoDB ping" },
    { label: "Email (SMTP)", ok: h.smtp.configured, value: h.smtp.configured ? "Configured" : "Not set up", detail: h.smtp.configured ? `${h.smtp.host} · from ${h.smtp.from ?? "default"}` : "Set SMTP_URL and EMAIL_FROM. Verification and reset emails are not delivered." },
    { label: "Email queue", ok: h.queue.retrying === 0 && (h.queue.oldestMinutes ?? 0) < 15, value: `${h.queue.pending} waiting`, detail: h.queue.retrying ? `${h.queue.retrying} failing and retrying. Oldest ${h.queue.oldestMinutes} min.` : h.queue.pending ? `Oldest ${h.queue.oldestMinutes} min` : "Empty" },
    { label: "Code runner", ok: h.runner.mode === "local" || h.runner.reachable === true, value: h.runner.mode === "socket" ? (h.runner.reachable ? "Service reachable" : "Service down") : "Local (dev)", detail: h.runner.mode === "socket" ? "RUNNER_SOCKET" : "No RUNNER_SOCKET. Production requires one." },
    { label: "AI tutor", ok: h.tutor.configured, value: h.tutor.configured ? `Ready (${h.tutor.provider})` : "No API key", detail: h.tutor.provider === "claude" ? "ANTHROPIC_API_KEY" : "Local Ollama model" },
    { label: "Auth secret", ok: h.secret.ok, value: h.secret.ok ? "Set" : "Too short or missing", detail: "AUTH_SECRET, 32+ characters" },
    { label: "Accounts", ok: h.accounts.adminsWithout2fa === 0, value: `${h.accounts.total} total`, detail: `${h.accounts.unverified} unverified · ${h.accounts.suspended} locked or banned${h.accounts.adminsWithout2fa ? ` · ${h.accounts.adminsWithout2fa} admin without 2FA` : ""}` },
  ];
  return (
    <section aria-labelledby="health">
      <h2 id="health" className="display-sm font-semibold">System health</h2>
      <div className="mt-3 grid gap-3 sm:grid-cols-2 lg:grid-cols-4">
        {items.map((i) => (
          <Card key={i.label} className="p-4">
            <p className="text-sm text-muted">{i.label}</p>
            <p className={`mt-1 font-semibold ${i.ok ? "text-ok" : "text-err"}`}>{i.ok ? "✓ " : "✗ "}{i.value}</p>
            <p className="mt-1 text-xs text-muted break-words">{i.detail}</p>
          </Card>
        ))}
      </div>
    </section>
  );
}

function StatTable({ title, head, rows }: { title: string; head: [string, string]; rows: (string | number)[][] }) {
  return (
    <Card as="section" className="p-6">
      <h2 className="display-sm font-semibold">{title}</h2>
      {rows.length === 0 ? (
        <p className="mt-3 text-sm text-muted">No data yet.</p>
      ) : (
        <table className="mt-3 w-full text-sm">
          <thead>
            <tr className="text-left text-muted">
              <th className="py-1.5 font-medium">{head[0]}</th>
              <th className="py-1.5 text-right font-medium">{head[1]}</th>
            </tr>
          </thead>
          <tbody>
            {rows.map(([a, b]) => (
              <tr key={String(a)} className="border-t border-border">
                <td className="py-1.5 font-mono text-xs break-all">{a}</td>
                <td className="py-1.5 text-right tabular-nums">{typeof b === "number" ? b.toLocaleString("en-GB") : b}</td>
              </tr>
            ))}
          </tbody>
        </table>
      )}
    </Card>
  );
}

function AdminSkeleton() {
  return (
    <div className="mt-8 space-y-6" aria-busy="true" aria-label="Loading analytics">
      <div className="grid gap-4 sm:grid-cols-2 lg:grid-cols-4">
        {[0, 1, 2, 3].map((i) => (
          <Skeleton key={i} className="h-24" />
        ))}
      </div>
      <Skeleton className="h-56" />
      <div className="grid gap-6 lg:grid-cols-2">
        <Skeleton className="h-48" />
        <Skeleton className="h-48" />
      </div>
    </div>
  );
}
