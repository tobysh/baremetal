import Link from "next/link";
import { Skeleton } from "@/components/ui";

export function SettingsNav({ current }: { current: "account" | "security" }) {
  const item = (id: typeof current, href: string, label: string) => (
    <Link
      href={href}
      aria-current={current === id ? "page" : undefined}
      className={`-mb-px inline-flex min-h-11 items-center border-b-2 px-3 py-2 text-sm ${current === id ? "border-accent font-medium text-text" : "border-transparent text-muted hover:text-text"}`}
    >
      {label}
    </Link>
  );
  return (
    <nav aria-label="Settings" className="mt-6 flex border-b border-border">
      {item("account", "/settings", "Account & privacy")}
      {item("security", "/settings/security", "Security")}
    </nav>
  );
}

export function SettingsSkeleton() {
  return (
    <div className="mt-8 space-y-8" aria-busy="true" aria-label="Loading settings">
      <Skeleton className="h-64" />
      <Skeleton className="h-80" />
      <Skeleton className="h-40" />
    </div>
  );
}
