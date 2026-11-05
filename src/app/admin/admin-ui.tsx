import Link from "next/link";
import type { AdminUserRow } from "@/lib/admin";

/** Deterministic UTC timestamp, so server and client render the same text. */
export const fmt = (d: Date | null | undefined) => (d ? `${d.toISOString().slice(0, 16).replace("T", " ")} UTC` : "never");
export const usd = (n: number) => `$${n.toFixed(n < 1 ? 3 : 2)}`;

export function StatusBadge({ status }: { status: AdminUserRow["status"] }) {
  const tone = status === "active" ? "border-transparent bg-ok-soft text-ok" : status === "locked" ? "border-transparent bg-accent-soft text-accent" : "border-transparent bg-err-soft text-err";
  return <span className={`inline-flex items-center rounded-full border px-2.5 py-0.5 text-xs font-medium ${tone}`}>{status === "active" ? "Active" : status === "locked" ? "Locked" : "Banned"}</span>;
}

export function Pager({ page, pages, href }: { page: number; pages: number; href: (p: number) => string }) {
  if (pages <= 1) return null;
  const link = "rounded-lg border border-border px-3 py-1.5 text-sm hover:border-accent";
  return (
    <nav aria-label="Pages" className="mt-4 flex items-center gap-3 text-sm">
      {page > 1 && <Link href={href(page - 1)} className={link}>Previous</Link>}
      <span className="text-muted">Page {page} of {pages}</span>
      {page < pages && <Link href={href(page + 1)} className={link}>Next</Link>}
    </nav>
  );
}

export const qs = (params: Record<string, string | number | undefined>) => {
  const u = new URLSearchParams();
  for (const [k, v] of Object.entries(params)) if (v !== undefined && v !== "" && v !== "all" && v !== 1) u.set(k, String(v));
  const s = u.toString();
  return s ? `?${s}` : "";
};

export const first = (v: string | string[] | undefined) => (Array.isArray(v) ? v[0] : v) ?? "";
