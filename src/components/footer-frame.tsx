"use client";

import { usePathname } from "next/navigation";
import type { ReactNode } from "react";

export function FooterFrame({ children, legal }: { children: ReactNode; legal: ReactNode }) {
  const path = usePathname();
  const workspace = /^\/learn\/[^/]+\/[^/]+\/?$/.test(path);
  // Lesson pages carry their own one-line legal strip inside the workspace shell.
  if (workspace) return null;
  return <footer className="mt-16 border-t border-border sm:mt-24">{children}{legal}</footer>;
}
