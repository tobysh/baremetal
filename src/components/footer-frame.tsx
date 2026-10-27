"use client";

import { usePathname } from "next/navigation";
import type { ReactNode } from "react";

export function FooterFrame({ children, legal }: { children: ReactNode; legal: ReactNode }) {
  const path = usePathname();
  const workspace = /^\/learn\/[^/]+\/[^/]+\/?$/.test(path);
  return <footer className={`${workspace ? "mt-6" : "mt-16 sm:mt-24"} border-t border-border`}>{!workspace && children}{legal}</footer>;
}
