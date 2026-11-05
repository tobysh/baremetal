import type { Metadata } from "next";
import Link from "next/link";
import { notFound } from "next/navigation";
import { Suspense } from "react";
import { connection } from "next/server";
import { Card, Container, Skeleton } from "@/components/ui";
import { getAdminUser, listAdminAudit } from "@/lib/admin";
import { requireAdmin } from "@/lib/auth";
import { fmt, StatusBadge } from "../../admin-ui";
import { UserActions } from "../../user-forms";

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
    ["Active sessions", String(u.activeSessions)],
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

      <Card as="section" className="p-6">
        <h2 className="display-sm font-semibold">Actions</h2>
        <div className="mt-4">
          {editable ? <UserActions userId={u.id} email={u.email} status={u.status} unverified={!u.emailVerified} /> : <p className="text-sm text-muted">Admin accounts can only be changed from the command line (<code>npm run make-admin</code>).</p>}
        </div>
      </Card>

      <div className="grid gap-6 lg:grid-cols-2">
        <Card as="section" className="p-6">
          <h2 className="display-sm font-semibold">Recent security events</h2>
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
