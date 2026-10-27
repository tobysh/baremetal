import type { ReactNode } from "react";
import Link from "next/link";

export function AuthShell({ title, intro, children, footer }: { title: string; intro?: ReactNode; children: ReactNode; footer?: ReactNode }) {
  return (
    <div className="mx-auto w-full max-w-lg px-4 py-10 sm:px-6 sm:py-16">
      <Link href="/learn" className="inline-flex min-h-11 items-center text-sm text-muted hover:text-text">← Back to courses</Link>
      <h1 className="display mt-5 text-3xl font-bold sm:text-4xl">{title}</h1>
      {intro && <div className="mt-3 leading-relaxed text-muted">{intro}</div>}
      <div className="mt-8 panel p-5 sm:p-8">{children}</div>
      {footer && <div className="mt-6 text-center text-sm text-muted">{footer}</div>}
    </div>
  );
}
