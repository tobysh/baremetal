import type { Metadata } from "next";
import Link from "next/link";
import { Suspense } from "react";
import { connection } from "next/server";
import { Card, Container, Skeleton } from "@/components/ui";
import { searchUsers, USER_FILTERS, USER_SORTS, type SortDir, type UserFilter, type UserSort } from "@/lib/admin";
import { requireAdmin } from "@/lib/auth";
import { first, fmt, Pager, qs, StatusBadge } from "../admin-ui";
import { CreateAccountForm } from "../user-forms";

export const metadata: Metadata = {
  title: "Admin: users",
  description: "Find, create, lock, ban and delete accounts.",
  alternates: { canonical: "/admin/users" },
  robots: { index: false, follow: false },
};

export default function UsersPage({ searchParams }: PageProps<"/admin/users">) {
  return (
    <Container className="py-12">
      <h1 className="display text-3xl font-bold">Users</h1>
      <Suspense fallback={<Skeleton className="mt-8 h-72" />}>
        <UsersBody searchParams={searchParams} />
      </Suspense>
    </Container>
  );
}

async function UsersBody({ searchParams }: { searchParams: PageProps<"/admin/users">["searchParams"] }) {
  await connection();
  await requireAdmin();
  const sp = await searchParams;
  const q = first(sp.q);
  const f = first(sp.filter);
  const filter: UserFilter = USER_FILTERS.some((x) => x.value === f) ? (f as UserFilter) : "all";
  const s = first(sp.sort);
  const sort: UserSort = (USER_SORTS as readonly string[]).includes(s) ? (s as UserSort) : "joined";
  const dir: SortDir = first(sp.dir) === "asc" ? "asc" : "desc";
  const { users, total, page, pages } = await searchUsers(q, filter, Number(first(sp.page)) || 1, sort, dir);
  const state = { q, filter, sort: sort === "joined" ? undefined : sort, dir: dir === "desc" ? undefined : dir };
  const sortHref = (key: UserSort) => {
    const next: SortDir = sort === key && dir === "desc" ? "asc" : "desc";
    return `/admin/users${qs({ q, filter, sort: key === "joined" && next === "desc" ? undefined : key, dir: next === "desc" ? undefined : next })}`;
  };
  const sortTh = (k: UserSort, label: string, right?: boolean) => (
    <th key={k} scope="col" aria-sort={sort === k ? (dir === "asc" ? "ascending" : "descending") : "none"} className={`px-4 py-2 font-medium ${right ? "text-right" : ""}`}>
      <Link href={sortHref(k)} className="underline-offset-2 hover:text-text hover:underline">
        {label}<span aria-hidden="true">{sort === k ? (dir === "asc" ? " ▲" : " ▼") : ""}</span>
      </Link>
    </th>
  );
  return (
    <div className="mt-6 space-y-6">
      {first(sp.deleted) && <p role="status" className="rounded-lg bg-ok-soft px-4 py-3 text-sm text-ok">Account deleted.</p>}
      <form method="get" className="flex flex-wrap items-end gap-3">
        <div>
          <label htmlFor="q" className="block text-sm font-medium">Search</label>
          <input id="q" name="q" defaultValue={q} placeholder="Email, username, name or id" className="mt-1.5 block h-11 w-72 max-w-full rounded-lg border border-field bg-bg px-3" />
        </div>
        <div>
          <label htmlFor="filter" className="block text-sm font-medium">Show</label>
          <select id="filter" name="filter" defaultValue={filter} className="mt-1.5 block h-11 rounded-lg border border-field bg-bg px-3">
            {USER_FILTERS.map((x) => <option key={x.value} value={x.value}>{x.label}</option>)}
          </select>
        </div>
        {state.sort && <input type="hidden" name="sort" value={state.sort} />}
        {state.dir && <input type="hidden" name="dir" value={state.dir} />}
        <button type="submit" className="inline-flex h-11 items-center rounded-lg bg-accent px-5 font-medium text-accent-ink hover:brightness-110">Search</button>
      </form>

      <Card className="overflow-x-auto p-0">
        <table className="w-full text-sm">
          <caption className="sr-only">Accounts</caption>
          <thead>
            <tr className="text-left text-muted">
              <th scope="col" className="px-4 py-2 font-medium">Account</th>
              <th scope="col" className="px-4 py-2 font-medium">Status</th>
              <th scope="col" className="px-4 py-2 font-medium">Email</th>
              {sortTh("xp", "XP", true)}
              {sortTh("joined", "Joined")}
              {sortTh("lastLogin", "Last sign-in")}
            </tr>
          </thead>
          <tbody>
            {users.length === 0 && <tr><td colSpan={6} className="px-4 py-6 text-muted">No accounts match.</td></tr>}
            {users.map((u) => (
              <tr key={u.id} className="border-t border-border">
                <td className="px-4 py-2">
                  <Link href={`/admin/users/${u.id}`} className="font-medium text-accent underline underline-offset-2">{u.username ?? u.name}</Link>
                  {u.role === "admin" && <span className="ml-2 text-xs text-muted">admin</span>}
                  <div className="text-xs text-muted">{u.name}</div>
                </td>
                <td className="px-4 py-2"><StatusBadge status={u.status} /></td>
                <td className="px-4 py-2 font-mono text-xs break-all">{u.email}{!u.emailVerified && <span className="ml-2 font-sans text-muted">(unverified)</span>}</td>
                <td className="px-4 py-2 text-right tabular-nums">{u.xp.toLocaleString("en-GB")}</td>
                <td className="px-4 py-2 whitespace-nowrap">{fmt(u.createdAt).slice(0, 10)}</td>
                <td className="px-4 py-2 whitespace-nowrap">{u.lastLogin ? fmt(u.lastLogin) : <span className="text-muted">never</span>}</td>
              </tr>
            ))}
          </tbody>
        </table>
      </Card>
      <p className="text-sm text-muted">{total.toLocaleString("en-GB")} account{total === 1 ? "" : "s"}</p>
      <Pager page={page} pages={pages} href={(p) => `/admin/users${qs({ ...state, page: p })}`} />

      <Card as="section" className="p-6">
        <h2 className="display-sm font-semibold">Create an account</h2>
        <div className="mt-4"><CreateAccountForm /></div>
      </Card>
    </div>
  );
}
