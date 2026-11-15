import type { Metadata } from "next";
import Link from "next/link";
import { Suspense } from "react";
import { connection } from "next/server";
import { Card, Container, Skeleton } from "@/components/ui";
import { getSystemHealth, getTutorCost, type Health } from "@/lib/admin";
import { requireAdmin } from "@/lib/auth";
import { getPopularTutorAnswers } from "@/lib/tutor-popular";
import { failingChecks, healthItems } from "@/lib/admin-health";
import { getHeadlineTrend, getSiteStats } from "@/lib/stats";
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
  const [s, popular, health, cost, trend] = await Promise.all([getSiteStats(days), getPopularTutorAnswers(20), getSystemHealth(), getTutorCost(days), getHeadlineTrend(days)]);
  const checkedAt = new Date();
  const maxDaily = Math.max(1, ...s.daily.map((d) => d.views));
  const dayKeys = lastDays(days);
  const series = (rows: { day: string; n: number }[]) => {
    const m = new Map(rows.map((r) => [r.day, r.n]));
    return dayKeys.map((k) => m.get(k) ?? 0);
  };
  const dailyMap = new Map(s.daily.map((d) => [d.day, d]));
  const headline: { label: string; value: number; prior: number; spark: number[] }[] = [
    { label: "Pageviews", value: s.pageviews, prior: trend.prior.pageviews, spark: dayKeys.map((k) => dailyMap.get(k)?.views ?? 0) },
    { label: "Unique visitors (daily)", value: s.visitors, prior: trend.prior.visitors, spark: dayKeys.map((k) => dailyMap.get(k)?.visitors ?? 0) },
    { label: "Sign-ups", value: s.funnel.signups, prior: trend.prior.signups, spark: series(trend.signupsDaily) },
    { label: "Lessons passed", value: s.funnel.testPasses, prior: trend.prior.testPasses, spark: series(trend.passesDaily) },
  ];
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

      <HealthPanel h={health} checkedAt={checkedAt} />

      <div className="grid gap-4 sm:grid-cols-2 lg:grid-cols-4">
        {headline.map((c) => (
          <Card key={c.label} className="p-5">
            <p className="text-sm text-muted">{c.label}</p>
            <p className="mt-1 text-3xl font-semibold tabular-nums">{c.value.toLocaleString("en-GB")}</p>
            <Delta value={c.value} prior={c.prior} days={days} />
            <Sparkline values={c.spark} label={c.label} />
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
          <>
            <p className="mt-2 flex flex-wrap gap-x-4 text-xs text-muted">
              <span><span aria-hidden="true" className="mr-1 inline-block size-2 rounded-sm bg-accent" />Pageviews</span>
              <span><span aria-hidden="true" className="mr-1 inline-block size-2 rounded-sm bg-text" />Unique visitors</span>
              <span>Peak: {maxDaily.toLocaleString("en-GB")} views</span>
            </p>
            <div className="mt-3 flex">
              <div className="flex h-40 w-10 shrink-0 flex-col justify-between pr-2 text-right text-xs tabular-nums text-muted" aria-hidden="true">
                <span>{maxDaily.toLocaleString("en-GB")}</span>
                <span>0</span>
              </div>
              <div className="flex h-40 flex-1 items-end gap-1 border-b border-border" role="img" aria-label={`Daily pageviews and unique visitors, ${s.daily.length} days with data, peak ${maxDaily} views`}>
                {s.daily.map((d) => (
                  <div key={d.day} className="relative flex h-full flex-1 items-end" title={`${d.day}: ${d.views} views, ${d.visitors} visitors`}>
                    <div className="relative w-full rounded-t bg-accent" style={{ height: `${(d.views / maxDaily) * 100}%` }}>
                      <div className="absolute inset-x-1/4 bottom-0 rounded-t bg-text" style={{ height: `${d.views ? Math.min(100, (d.visitors / d.views) * 100) : 0}%` }} />
                    </div>
                  </div>
                ))}
              </div>
            </div>
            <p className="mt-1 flex justify-between pl-10 text-xs text-muted">
              <span>{s.daily[0].day}</span>
              <span>{s.daily[s.daily.length - 1].day}</span>
            </p>
            <details className="mt-3 text-sm">
              <summary className="cursor-pointer text-muted">Show data table</summary>
              <div className="mt-2 max-h-72 overflow-auto">
                <table className="w-full text-sm">
                  <caption className="sr-only">Daily pageviews and unique visitors</caption>
                  <thead><tr className="text-left text-muted"><th scope="col" className="py-1 font-medium">Day</th><th scope="col" className="py-1 text-right font-medium">Pageviews</th><th scope="col" className="py-1 text-right font-medium">Visitors</th></tr></thead>
                  <tbody>
                    {s.daily.map((d) => (
                      <tr key={d.day} className="border-t border-border"><th scope="row" className="py-1 text-left font-mono text-xs font-normal">{d.day}</th><td className="py-1 text-right tabular-nums">{d.views.toLocaleString("en-GB")}</td><td className="py-1 text-right tabular-nums">{d.visitors.toLocaleString("en-GB")}</td></tr>
                    ))}
                  </tbody>
                </table>
              </div>
            </details>
          </>
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

function HealthPanel({ h, checkedAt }: { h: Health; checkedAt: Date }) {
  const items = healthItems(h);
  const failing = failingChecks(h).length;
  return (
    <section aria-labelledby="health">
      <div className="flex flex-wrap items-baseline gap-x-3">
        <h2 id="health" className="display-sm font-semibold">System health</h2>
        <p className={`text-sm font-medium ${failing ? "text-err" : "text-ok"}`}>{failing ? `${failing} ${failing === 1 ? "check" : "checks"} failing` : "All checks passing"}</p>
        <p className="text-xs text-muted">Last checked <time dateTime={checkedAt.toISOString()}>{checkedAt.toISOString().slice(11, 19)} UTC</time></p>
      </div>
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

function lastDays(n: number): string[] {
  const out: string[] = [];
  const now = Date.now();
  for (let i = n - 1; i >= 0; i--) out.push(new Date(now - i * 86_400_000).toISOString().slice(0, 10));
  return out;
}

function Delta({ value, prior, days }: { value: number; prior: number; days: number }) {
  const vs = `vs prior ${days}d`;
  if (prior === 0) {
    return <p className="mt-1 text-xs text-muted">{value === 0 ? `No change ${vs}` : `New, none ${vs}`}</p>;
  }
  const pct = Math.round(((value - prior) / prior) * 100);
  if (pct === 0) return <p className="mt-1 text-xs text-muted">{`No change ${vs} (${prior.toLocaleString("en-GB")})`}</p>;
  const up = pct > 0;
  return (
    <p className={`mt-1 text-xs font-medium ${up ? "text-ok" : "text-err"}`}>
      <span aria-hidden="true">{up ? "▲" : "▼"} </span>
      {up ? "+" : "−"}{Math.abs(pct).toLocaleString("en-GB")}% {vs}
      <span className="sr-only"> ({up ? "increase" : "decrease"} from {prior.toLocaleString("en-GB")})</span>
    </p>
  );
}

function Sparkline({ values, label }: { values: number[]; label: string }) {
  if (values.length < 2 || values.every((v) => v === 0)) return null;
  const w = 100, h = 24;
  const max = Math.max(...values);
  const pts = values.map((v, i) => `${((i / (values.length - 1)) * w).toFixed(1)},${(h - 2 - (v / max) * (h - 4)).toFixed(1)}`).join(" ");
  return (
    <svg viewBox={`0 0 ${w} ${h}`} preserveAspectRatio="none" className="mt-3 h-6 w-full text-accent" role="img" aria-label={`${label} daily trend, peak ${max.toLocaleString("en-GB")}`}>
      <polyline points={pts} fill="none" stroke="currentColor" strokeWidth="1.5" vectorEffect="non-scaling-stroke" strokeLinejoin="round" />
    </svg>
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
