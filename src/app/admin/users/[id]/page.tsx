import type { Metadata } from "next";
import Link from "next/link";
import { notFound } from "next/navigation";
import { Suspense } from "react";
import { connection } from "next/server";
import { Card, Container, Skeleton } from "@/components/ui";
import { getAdminUser, listAdminAudit } from "@/lib/admin";
import { requireAdmin } from "@/lib/auth";
import { fmt, StatusBadge } from "../../admin-ui";
import { RevokeSessionForm, UserActions } from "../../user-forms";

export const metadata: Metadata = { title: "Admin: account", description: "Account details and actions.", robots: { index: false, follow: false } };

export default function UserPage({ params }: PageProps<"/admin/users/[id]">) {
  return (
    <Container className="py-12">
      <p className="text-sm"><Link href="/admin/users" className="text-accent underline underline-offset-2">All users</Link></p>
      <Suspense fallback={<Skeleton className="mt-6 h-96" />}>
        <UserBody params={params} />
      </Suspense>
    </Container>
  );
}

async function UserBody({ params }: { params: PageProps<"/admin/users/[id]">["params"] }) {
  await connection();
  const admin = await requireAdmin();
  const { id } = await params;
  const [u, audit] = await Promise.all([getAdminUser(id), listAdminAudit({ targetId: id, limit: 20 })]);
  if (!u) notFound();
  const editable = u.role !== "admin" && u.id !== admin.id;
  const rows: [string, string][] = [
    ["Email", `${u.email}${u.emailVerified ? "" : " (not verified)"}`],
    ["Name", u.name],
    ["Joined", fmt(u.createdAt)],
    ["Last sign-in", fmt(u.lastLogin)],
    ["Two-factor", u.twoFactor ? "On" : "Off"],
    ["XP", u.xp.toLocaleString("en-GB")],
    ["Lessons passed", String(u.passed)],
    ["Experience", u.experience],
    ["Pending links", [u.pendingReset && "password reset", u.pendingVerification && "verification"].filter(Boolean).join(", ") || "none"],
  ];
  return (
    <div className="mt-4 space-y-6">
      <div className="flex flex-wrap items-center gap-3">
        <h1 className="display text-3xl font-bold">{u.username ?? u.name}</h1>
        <StatusBadge status={u.status} />
        {u.role === "admin" && <span className="text-sm text-muted">admin</span>}
      </div>
      {u.suspension && (
        <p role="status" className="rounded-lg bg-err-soft px-4 py-3 text-sm text-err">
          {u.suspension.kind === "banned" ? "Banned" : "Locked"} on {fmt(u.suspension.at)}
          {u.suspension.until ? `, until ${fmt(u.suspension.until)}` : u.suspension.kind === "locked" ? ", until unlocked" : ""}
          {u.suspension.reason ? `. Note: ${u.suspension.reason}` : ""}
        </p>
      )}
      <Card as="section" className="p-6">
        <h2 className="sr-only">Details</h2>
        <dl className="grid gap-x-8 gap-y-3 text-sm sm:grid-cols-2">
          {rows.map(([k, v]) => (
            <div key={k} className="flex justify-between gap-4 border-b border-border pb-2">
              <dt className="text-muted">{k}</dt>
              <dd className="text-right break-all">{v}</dd>
            </div>
          ))}
        </dl>
      </Card>

      <div className="grid gap-6 lg:grid-cols-2">
        <Card as="section" className="p-6">
          <h2 className="display-sm font-semibold">Progress</h2>
          {u.progress.tracks.length === 0 ? <p className="mt-3 text-sm text-muted">No track started yet.</p> : (
            <ul className="mt-3 space-y-1 text-sm">
              {u.progress.tracks.map((t) => (
                <li key={t.slug} className="flex justify-between gap-4"><span>{t.title}</span><span className="text-muted">{t.passed} / {t.total} passed</span></li>
              ))}
            </ul>
          )}
          {u.progress.recent.length > 0 && (
            <>
              <h3 className="mt-4 text-sm font-medium">Latest activity</h3>
              <ul className="mt-2 space-y-2 text-sm">
                {u.progress.recent.map((r, i) => (
                  <li key={i} className="flex flex-wrap justify-between gap-2 border-t border-border pt-2">
                    <span><Link href={r.href} className="text-accent underline underline-offset-2">{r.title}</Link> <span className="text-muted">({r.status === "passed" ? "passed" : "attempted"})</span></span>
                    <span className="text-muted">{fmt(r.at)}</span>
                  </li>
                ))}
              </ul>
            </>
          )}
          <p className="mt-4 text-sm text-muted">Tutor questions: {u.progress.tutorQuestions}</p>
          {u.profilePublic && u.username && <p className="mt-2 text-sm"><Link href={`/u/${u.username}`} className="text-accent underline underline-offset-2">Public profile</Link></p>}
        </Card>

        <Card as="section" className="p-6">
          <h2 className="display-sm font-semibold">Active sessions ({u.activeSessions})</h2>
          {u.sessions.length === 0 ? <p className="mt-3 text-sm text-muted">No active sessions.</p> : (
            <ul className="mt-3 space-y-3 text-sm">
              {u.sessions.map((s) => (
                <li key={s.id} className="flex flex-wrap items-center justify-between gap-2 border-t border-border pt-3">
                  <div>
                    <div className="font-medium">{s.device}</div>
                    <div className="text-muted">Signed in {fmt(s.createdAt)} · last seen {fmt(s.lastSeenAt)}</div>
                  </div>
                  {editable && <RevokeSessionForm userId={u.id} sessionId={s.id} />}
                </li>
              ))}
            </ul>
          )}
          <p className="mt-3 text-xs text-muted">IP addresses are not stored with sessions; see security events for hashed addresses.</p>
        </Card>
      </div>

      <Card as="section" className="p-6">
        <h2 className="display-sm font-semibold">Actions</h2>
        <div className="mt-4">
          {editable ? <UserActions userId={u.id} email={u.email} status={u.status} unverified={!u.emailVerified} /> : <p className="text-sm text-muted">Admin accounts can only be changed from the command line (<code>npm run make-admin</code>).</p>}
        </div>
      </Card>

      <div className="grid gap-6 lg:grid-cols-2">
        <Card as="section" className="p-6">
          <h2 className="display-sm font-semibold">Recent security events</h2>
          <p className="mt-1 text-sm"><Link href={`/admin/security?q=${encodeURIComponent(u.email)}`} className="text-accent underline underline-offset-2">All events for this user</Link></p>
          {u.events.length === 0 ? <p className="mt-3 text-sm text-muted">Nothing in the last 90 days.</p> : (
            <ul className="mt-3 space-y-2 text-sm">
              {u.events.map((e, i) => (
                <li key={i} className="flex flex-wrap justify-between gap-2 border-t border-border pt-2">
                  <span>{e.type}{e.reason ? ` (${e.reason})` : ""}</span>
                  <span className="text-muted">{fmt(e.ts)} · ip {e.ipHash}</span>
                </li>
              ))}
            </ul>
          )}
        </Card>
        <Card as="section" className="p-6">
          <h2 className="display-sm font-semibold">Admin actions on this account</h2>
          <p className="mt-1 text-sm"><Link href={`/admin/security?q=${encodeURIComponent(u.email)}`} className="text-accent underline underline-offset-2">All events for this user</Link></p>
          {audit.length === 0 ? <p className="mt-3 text-sm text-muted">None recorded.</p> : (
            <ul className="mt-3 space-y-2 text-sm">
              {audit.map((a, i) => (
                <li key={i} className="border-t border-border pt-2">
                  <span className="font-medium">{a.label}</span> by {a.admin}
                  <div className="text-muted">{fmt(a.ts)}{a.note ? ` · ${a.note}` : ""}</div>
                </li>
              ))}
            </ul>
          )}
        </Card>
      </div>
    </div>
  );
}
