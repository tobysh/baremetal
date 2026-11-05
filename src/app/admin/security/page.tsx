import type { Metadata } from "next";
import { Suspense } from "react";
import { connection } from "next/server";
import { Card, Container, Skeleton } from "@/components/ui";
import { AUTH_EVENT_TYPES, listAdminAudit, listAuthEvents } from "@/lib/admin";
import { requireAdmin } from "@/lib/auth";
import { first, fmt, Pager, qs } from "../admin-ui";
import Link from "next/link";

export const metadata: Metadata = {
  title: "Admin: security log",
  description: "Sign-in events and admin actions.",
  alternates: { canonical: "/admin/security" },
  robots: { index: false, follow: false },
};

export default function SecurityPage({ searchParams }: PageProps<"/admin/security">) {
  return (
    <Container className="py-12">
      <h1 className="display text-3xl font-bold">Security log</h1>
      <p className="mt-2 text-muted">Sign-in events are kept 90 days; admin actions one year. IP addresses are shown as a short keyed hash.</p>
      <Suspense fallback={<Skeleton className="mt-8 h-96" />}>
        <SecurityBody searchParams={searchParams} />
      </Suspense>
    </Container>
  );
}

async function SecurityBody({ searchParams }: { searchParams: PageProps<"/admin/security">["searchParams"] }) {
  await connection();
  await requireAdmin();
  const sp = await searchParams;
  const type = first(sp.type);
  const q = first(sp.q);
  const [{ rows, page, pages, total }, audit] = await Promise.all([listAuthEvents(type, q, Number(first(sp.page)) || 1), listAdminAudit({ limit: 30 })]);
  return (
    <div className="mt-6 space-y-6">
      <form method="get" className="flex flex-wrap items-end gap-3">
        <div>
          <label htmlFor="type" className="block text-sm font-medium">Event</label>
          <select id="type" name="type" defaultValue={type} className="mt-1.5 block h-11 rounded-lg border border-field bg-bg px-3">
            <option value="">All events</option>
            {AUTH_EVENT_TYPES.map((t) => <option key={t} value={t}>{t}</option>)}
          </select>
        </div>
        <div>
          <label htmlFor="q" className="block text-sm font-medium">Email contains</label>
          <input id="q" name="q" defaultValue={q} className="mt-1.5 block h-11 w-64 max-w-full rounded-lg border border-field bg-bg px-3" />
        </div>
        <button type="submit" className="inline-flex h-11 items-center rounded-lg bg-accent px-5 font-medium text-accent-ink hover:brightness-110">Filter</button>
      </form>

      <Card className="overflow-x-auto p-0">
        <table className="w-full text-sm">
          <caption className="sr-only">Security events</caption>
          <thead>
            <tr className="text-left text-muted">
              <th scope="col" className="px-4 py-2 font-medium">Time</th>
              <th scope="col" className="px-4 py-2 font-medium">Event</th>
              <th scope="col" className="px-4 py-2 font-medium">Email</th>
              <th scope="col" className="px-4 py-2 font-medium">IP hash</th>
            </tr>
          </thead>
          <tbody>
            {rows.length === 0 && <tr><td colSpan={4} className="px-4 py-6 text-muted">No events.</td></tr>}
            {rows.map((e, i) => (
              <tr key={i} className="border-t border-border">
                <td className="px-4 py-2 whitespace-nowrap">{fmt(e.ts)}</td>
                <td className="px-4 py-2">{e.type}{e.reason ? <span className="text-muted"> ({e.reason})</span> : null}</td>
                <td className="px-4 py-2 font-mono text-xs break-all">{e.userId ? <Link href={`/admin/users/${e.userId}`} className="text-accent underline underline-offset-2">{e.email ?? e.userId}</Link> : (e.email ?? "")}</td>
                <td className="px-4 py-2 font-mono text-xs">{e.ipHash}</td>
              </tr>
            ))}
          </tbody>
        </table>
      </Card>
      <p className="text-sm text-muted">{total.toLocaleString("en-GB")} events</p>
      <Pager page={page} pages={pages} href={(p) => `/admin/security${qs({ type, q, page: p })}`} />

      <Card as="section" className="p-6">
        <h2 className="display-sm font-semibold">Admin actions</h2>
        {audit.length === 0 ? <p className="mt-3 text-sm text-muted">No admin actions yet.</p> : (
          <ul className="mt-3 space-y-2 text-sm">
            {audit.map((a, i) => (
              <li key={i} className="border-t border-border pt-2">
                <span className="font-medium">{a.label}</span> by {a.admin}
                {a.targetId && <> on <Link href={`/admin/users/${a.targetId}`} className="text-accent underline underline-offset-2">{a.target}</Link></>}
                <div className="text-muted">{fmt(a.ts)}{a.note ? ` · ${a.note}` : ""}</div>
              </li>
            ))}
          </ul>
        )}
      </Card>
    </div>
  );
}
