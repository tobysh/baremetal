import type { Metadata } from "next";
import { Suspense } from "react";
import { Card, Container, Skeleton } from "@/components/ui";
import { requireAdmin } from "@/lib/auth";
import { getSiteStats } from "@/lib/stats";

export const metadata: Metadata = {
  title: "Admin: analytics",
  description: "Site analytics and security overview.",
  robots: { index: false, follow: false },
};

export default function AdminPage() {
  return (
    <Container className="py-12">
      <h1 className="display text-3xl font-bold">Analytics</h1>
      <p className="mt-2 text-muted">Last 30 days · consented visitors only · no raw IPs stored</p>
      <Suspense fallback={<AdminSkeleton />}>
        <AdminBody />
      </Suspense>
    </Container>
  );
}

async function AdminBody() {
  // Authorization happens here, on the server: role must be admin AND 2FA must be on.
  await requireAdmin();
  const s = await getSiteStats(30);
  const maxDaily = Math.max(1, ...s.daily.map((d) => d.views));
  return (
    <div className="mt-8 space-y-6">
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
          title="AI tutor (30 days)"
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
          title="Security (30 days)"
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
                <td className="py-1.5 text-right tabular-nums">{Number(b).toLocaleString("en-GB")}</td>
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
