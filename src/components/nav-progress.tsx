"use client";

import { usePathname } from "next/navigation";
import { useEffect, useRef, useState } from "react";

type Phase = "idle" | "loading" | "done";

/** A thin bar that starts on the click of an internal link, so a navigation is acknowledged at once. */
export function NavProgress() {
  const pathname = usePathname();
  const [phase, setPhase] = useState<Phase>("idle");
  const timer = useRef<ReturnType<typeof setTimeout>>(undefined);

  useEffect(() => {
    const onClick = (event: MouseEvent) => {
      if (event.defaultPrevented || event.button !== 0 || event.metaKey || event.ctrlKey || event.shiftKey || event.altKey) return;
      const a = (event.target as Element | null)?.closest?.("a");
      if (!a || a.target || a.hasAttribute("download")) return;
      const url = new URL(a.href, location.href);
      if (url.origin !== location.origin || (url.pathname === location.pathname && url.search === location.search)) return;
      clearTimeout(timer.current);
      setPhase("loading");
      // Never leave the bar stuck if the navigation fails or is cancelled.
      timer.current = setTimeout(() => setPhase("idle"), 10000);
    };
    document.addEventListener("click", onClick);
    return () => document.removeEventListener("click", onClick);
  }, []);

  // The route changed: finish the bar (adjusting state during render, not in an effect).
  const [seen, setSeen] = useState(pathname);
  if (seen !== pathname) {
    setSeen(pathname);
    setPhase("done");
  }

  useEffect(() => {
    if (phase !== "done") return;
    const t = setTimeout(() => setPhase("idle"), 300);
    return () => clearTimeout(t);
  }, [phase]);

  return <div aria-hidden="true" className="nav-progress" data-phase={phase} />;
}
