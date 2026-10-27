import type { ReactNode } from "react";

export function AuthShell({ title, intro, children, footer }: { title: string; intro?: ReactNode; children: ReactNode; footer?: ReactNode }) {
  return (
    <div className="mx-auto w-full max-w-md px-4 py-12 sm:py-16">
      <h1 className="text-2xl font-bold tracking-tight sm:text-3xl">{title}</h1>
      {intro && <div className="mt-2 text-muted">{intro}</div>}
      <div className="mt-8 rounded-xl border border-border bg-surface p-6 sm:p-8">{children}</div>
      {footer && <div className="mt-6 text-center text-sm text-muted">{footer}</div>}
    </div>
  );
}
