"use client";

import dynamic from "next/dynamic";
import { Skeleton } from "@/components/ui";

// xterm is only fetched when this page renders; it never reaches the shared bundle.
const Terminal = dynamic(() => import("./terminal"), {
  ssr: false,
  loading: () => <Skeleton className="h-[28rem] w-full rounded-2xl" />,
});

export function ShellLoader() {
  return <Terminal />;
}
