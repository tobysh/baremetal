"use client";

import Link from "next/link";
import { usePathname } from "next/navigation";

const LINKS = [
  { href: "/admin", label: "Overview" },
  { href: "/admin/users", label: "Users" },
  { href: "/admin/security", label: "Security log" },
  { href: "/admin/stuck", label: "Where learners get stuck" },
];

export function AdminNav({ healthFailing = 0 }: { healthFailing?: number }) {
  const path = usePathname();
  return (
    <nav aria-label="Admin" className="flex flex-wrap gap-2 border-b border-border pb-3">
      {LINKS.map((l) => {
        const active = l.href === "/admin" ? path === "/admin" : path.startsWith(l.href);
        return (
          <Link key={l.href} href={l.href} aria-current={active ? "page" : undefined} className={`rounded-lg px-3 py-1.5 text-sm font-medium ${active ? "bg-accent-soft text-accent" : "text-muted hover:text-text"}`}>
            {l.label}
            {l.href === "/admin" && healthFailing > 0 && (
              <span className="ml-2 inline-flex items-center gap-1 rounded-full border border-err bg-err/10 px-1.5 py-0.5 text-xs font-semibold text-err">
                <span aria-hidden="true">!</span>
                {healthFailing}
                <span className="sr-only"> system health {healthFailing === 1 ? "check" : "checks"} failing</span>
              </span>
            )}
          </Link>
        );
      })}
    </nav>
  );
}
