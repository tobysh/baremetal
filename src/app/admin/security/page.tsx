import type { Metadata } from "next";
import { Suspense } from "react";
import { connection } from "next/server";
import { Card, Container, Skeleton } from "@/components/ui";
import { ADMIN_ACTION_LABEL, ADMIN_ACTIONS, AUTH_EVENT_INFO, AUTH_EVENT_TYPES, listAdminAuditPage, listAuthEvents, type AuthSeverity } from "@/lib/admin";
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

const SEVERITY: Record<AuthSeverity, { tone: string; word: string }> = {
  critical: { tone: "border-transparent bg-err-soft text-err", word: "Critical" },
  warning: { tone: "border-transparent bg-accent-soft text-accent", word: "Warning" },
  info: { tone: "border-border bg-surface-2 text-muted", word: "Info" },
  ok: { tone: "border-transparent bg-ok-soft text-ok", word: "OK" },
};

const field = "mt-1.5 block h-11 rounded-lg border border-field bg-bg px-3";
const dateOnly = (v: string) => (/^\d{4}-\d{2}-\d{2}$/.test(v) ? v : "");
const hexOnly = (v: string) => (/^[0-9a-f]{6,16}$/.test(v) ? v : "");
const link = "text-accent underline underline-offset-2";

async function SecurityBody({ searchParams }: { searchParams: PageProps<"/admin/security">["searchParams"] }) {
  await connection();
  await requireAdmin();
  const sp = await searchParams;
  const type = first(sp.type);
  const q = first(sp.q);
  const ip = hexOnly(first(sp.ip));
  const from = dateOnly(first(sp.from));
  const to = dateOnly(first(sp.to));
  const aAdmin = first(sp.aadmin);
  const aAction = first(sp.aaction);
  const page = Number(first(sp.page)) || 1;
  const apage = Number(first(sp.apage)) || 1;
  const [{ rows, page: cur, pages, total, summary }, audit] = await Promise.all([
    listAuthEvents({ type, q, ip, from, to }, page),
    listAdminAuditPage(aAdmin, aAction, apage),
  ]);
  // Every link carries the whole filter state, so paging one list never resets the other.
  const href = (o: Record<string, string | number | undefined> = {}) =>
    `/admin/security${qs({ type, q, ip, from, to, aadmin: aAdmin, aaction: aAction, page, apage, ...o })}`;
  const hashLink = (h: string) => href({ ip: h, page: 1 });
  return (
    <div className="mt-6 space-y-6">
      <form method="get" className="flex flex-wrap items-end gap-3">
        <input type="hidden" name="ip" value={ip} />
        <input type="hidden" name="aadmin" value={aAdmin} />
        <input type="hidden" name="aaction" value={aAction} />
        <div>
          <label htmlFor="type" className="block text-sm font-medium">Event</label>
          <select id="type" name="type" defaultValue={type} className={field}>
            <option value="">All events</option>
            {AUTH_EVENT_TYPES.map((t) => <option key={t} value={t}>{AUTH_EVENT_INFO[t]?.label ?? t}</option>)}
          </select>
        </div>
        <div>
          <label htmlFor="q" className="block text-sm font-medium">Email contains</label>
          <input id="q" name="q" defaultValue={q} className={`${field} w-64 max-w-full`} />
        </div>
        <div>
          <label htmlFor="from" className="block text-sm font-medium">From (UTC)</label>
          <input id="from" name="from" type="date" defaultValue={from} className={field} />
        </div>
        <div>
          <label htmlFor="to" className="block text-sm font-medium">To (UTC)</label>
          <input id="to" name="to" type="date" defaultValue={to} className={field} />
        </div>
        <button type="submit" className="inline-flex h-11 items-center rounded-lg bg-accent px-5 font-medium text-accent-ink hover:brightness-110">Filter</button>
        {(type || q || ip || from || to) && <Link href={href({ type: "", q: "", ip: "", from: "", to: "", page: 1 })} className={`${link} inline-flex h-11 items-center`}>Clear</Link>}
      </form>
      {ip && <p className="text-sm">Showing only IP hash <span className="font-mono">{ip}</span>. <Link href={href({ ip: "", page: 1 })} className={link}>Show all addresses</Link></p>}

      <Card as="section" className="p-6" aria-label="Summary for the current filter">
        <dl className="grid gap-6 sm:grid-cols-2 lg:grid-cols-4">
          <div>
            <dt className="text-sm text-muted">Failed sign-ins</dt>
            <dd className="mt-1 text-2xl font-semibold">{summary.failed.toLocaleString("en-GB")}</dd>
          </div>
          <div>
            <dt className="text-sm text-muted">Lockouts</dt>
            <dd className="mt-1 text-2xl font-semibold">{summary.locked.toLocaleString("en-GB")}</dd>
          </div>
          <div>
            <dt className="text-sm text-muted">Most targeted emails</dt>
            <dd className="mt-1 text-sm">
              {summary.topEmails.length === 0 ? <span className="text-muted">None</span> : (
                <ol className="space-y-1">{summary.topEmails.map((e) => (
                  <li key={e.email} className="flex justify-between gap-2"><Link href={href({ q: e.email, page: 1 })} className={`${link} font-mono text-xs break-all`}>{e.email}</Link><span>{e.count}</span></li>
                ))}</ol>
              )}
            </dd>
          </div>
          <div>
            <dt className="text-sm text-muted">Top IP hashes (failures, accounts tried)</dt>
            <dd className="mt-1 text-sm">
              {summary.topHashes.length === 0 ? <span className="text-muted">None</span> : (
                <ol className="space-y-1">{summary.topHashes.map((h) => (
                  <li key={h.ipHash} className="flex justify-between gap-2"><Link href={hashLink(h.ipHash)} className={`${link} font-mono text-xs`}>{h.ipHash}</Link><span>{h.count} / {h.accounts}</span></li>
                ))}</ol>
              )}
            </dd>
          </div>
        </dl>
      </Card>

      <Card className="overflow-x-auto p-0">
        <table className="w-full text-sm">
          <caption className="sr-only">Security events</caption>
          <thead>
            <tr className="text-left text-muted">
              <th scope="col" className="px-4 py-2 font-medium">Time</th>
              <th scope="col" className="px-4 py-2 font-medium">Event</th>
              <th scope="col" className="px-4 py-2 font-medium">Email</th>
              <th scope="col" className="px-4 py-2 font-medium">IP hash</th>
              <th scope="col" className="px-4 py-2 font-medium">Accounts tried</th>
            </tr>
          </thead>
          <tbody>
            {rows.length === 0 && <tr><td colSpan={5} className="px-4 py-6 text-muted">No events.</td></tr>}
            {rows.map((e, i) => {
              const info = AUTH_EVENT_INFO[e.type] ?? { label: e.type, severity: "info" as const };
              const sev = SEVERITY[info.severity];
              return (
                <tr key={i} className="border-t border-border">
                  <td className="px-4 py-2 whitespace-nowrap">{fmt(e.ts)}</td>
                  <td className="px-4 py-2">
                    <span className={`inline-flex items-center rounded-full border px-2.5 py-0.5 text-xs font-medium ${sev.tone}`}><span className="sr-only">{sev.word}: </span>{info.label}</span>
                    {e.reason ? <span className="text-muted"> ({e.reason})</span> : null}
                  </td>
                  <td className="px-4 py-2 font-mono text-xs break-all">{e.userId ? <Link href={`/admin/users/${e.userId}`} className={link}>{e.email ?? e.userId}</Link> : (e.email ?? "")}</td>
                  <td className="px-4 py-2 font-mono text-xs"><Link href={hashLink(e.ipHash)} className={link} title="Show only this IP hash">{e.ipHash}</Link></td>
                  <td className="px-4 py-2 whitespace-nowrap">
                    {e.ipAccounts >= 3
                      ? <span className="inline-flex items-center rounded-full border border-transparent bg-err-soft px-2.5 py-0.5 text-xs font-medium text-err">{e.ipAccounts} accounts, possible stuffing</span>
                      : <span className="text-muted">{e.ipAccounts}</span>}
                  </td>
                </tr>
              );
            })}
          </tbody>
        </table>
      </Card>
      <p className="text-sm text-muted">{total.toLocaleString("en-GB")} events</p>
      <Pager page={cur} pages={pages} href={(p) => href({ page: p })} />

      <Card as="section" className="p-6">
        <h2 className="display-sm font-semibold">Admin actions</h2>
        <form method="get" className="mt-3 flex flex-wrap items-end gap-3">
          <input type="hidden" name="type" value={type} />
          <input type="hidden" name="q" value={q} />
          <input type="hidden" name="ip" value={ip} />
          <input type="hidden" name="from" value={from} />
          <input type="hidden" name="to" value={to} />
          <input type="hidden" name="page" value={cur} />
          <div>
            <label htmlFor="aadmin" className="block text-sm font-medium">Admin</label>
            <select id="aadmin" name="aadmin" defaultValue={aAdmin} className={field}>
              <option value="">All admins</option>
              {audit.admins.map((a) => <option key={a.id} value={a.id}>{a.name}</option>)}
            </select>
          </div>
          <div>
            <label htmlFor="aaction" className="block text-sm font-medium">Action</label>
            <select id="aaction" name="aaction" defaultValue={aAction} className={field}>
              <option value="">All actions</option>
              {ADMIN_ACTIONS.map((a) => <option key={a} value={a}>{ADMIN_ACTION_LABEL[a]}</option>)}
            </select>
          </div>
          <button type="submit" className="inline-flex h-11 items-center rounded-lg bg-accent px-5 font-medium text-accent-ink hover:brightness-110">Filter</button>
        </form>
        {audit.rows.length === 0 ? <p className="mt-3 text-sm text-muted">No admin actions found.</p> : (
          <ul className="mt-3 space-y-2 text-sm">
            {audit.rows.map((a, i) => (
              <li key={i} className="border-t border-border pt-2">
                <span className="font-medium">{a.label}</span> by {a.admin}
                {a.targetId && <> on <Link href={`/admin/users/${a.targetId}`} className={link}>{a.target}</Link></>}
                <div className="text-muted">{fmt(a.ts)}{a.note ? ` · ${a.note}` : ""}</div>
              </li>
            ))}
          </ul>
        )}
        <p className="mt-3 text-sm text-muted">{audit.total.toLocaleString("en-GB")} actions</p>
        <Pager page={audit.page} pages={audit.pages} href={(p) => href({ apage: p })} />
      </Card>
    </div>
  );
}
