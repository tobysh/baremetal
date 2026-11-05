"use client";

import Link from "next/link";
import { usePathname } from "next/navigation";
import type { ReactElement } from "react";

type Tab = { href: string; label: string; match: (path: string) => boolean; icon: ReactElement };

const svg = (d: ReactElement) => (
  <svg aria-hidden="true" viewBox="0 0 24 24" width="22" height="22" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round">{d}</svg>
);
const under = (base: string) => (p: string) => p === base || p.startsWith(`${base}/`);

/**
 * Persistent primary navigation for signed-in learners below `lg`. Hidden inside a lesson (the editor needs the height; the lesson
 * has its own header and back link). body:has([data-tabbar]) in globals.css reserves room under the page content.
 */
export function MobileTabBar({ profileHref }: { profileHref: string }) {
  const pathname = usePathname();
  // /learn/<track>/<lesson> is the workspace.
  if (/^\/learn\/[^/]+\/[^/]+/.test(pathname)) return null;
  const tabs: Tab[] = [
    { href: "/dashboard", label: "Home", match: under("/dashboard"), icon: svg(<path d="M4 11 12 4l8 7v8a1 1 0 0 1-1 1h-4v-6H9v6H5a1 1 0 0 1-1-1z" />) },
    { href: "/learn", label: "Courses", match: under("/learn"), icon: svg(<><path d="M4 5a1 1 0 0 1 1-1h5a2 2 0 0 1 2 2v14a2 2 0 0 0-2-2H4z" /><path d="M20 5a1 1 0 0 0-1-1h-5a2 2 0 0 0-2 2v14a2 2 0 0 1 2-2h6z" /></>) },
    { href: "/review", label: "Review", match: under("/review"), icon: svg(<><path d="M4 12a8 8 0 0 1 14-5.3L20 9" /><path d="M20 4v5h-5" /><path d="M20 12a8 8 0 0 1-14 5.3L4 15" /><path d="M4 20v-5h5" /></>) },
    { href: "/leaderboard", label: "Ranks", match: under("/leaderboard"), icon: svg(<><path d="M7 20V11M12 20V4M17 20v-6" /></>) },
    { href: profileHref, label: "Profile", match: (p) => under("/settings")(p) || (profileHref.startsWith("/u/") ? under(profileHref)(p) : false), icon: svg(<><circle cx="12" cy="8" r="4" /><path d="M4 21a8 8 0 0 1 16 0" /></>) },
  ];
  return (
    <nav data-tabbar aria-label="Primary" className="fixed inset-x-0 bottom-0 z-40 border-t border-border bg-bg pb-[env(safe-area-inset-bottom)] lg:hidden">
      <ul className="mx-auto grid max-w-xl grid-cols-5">
        {tabs.map((t) => {
          const active = t.match(pathname);
          return (
            <li key={t.label}>
              <Link href={t.href} aria-current={active ? "page" : undefined} className={`flex min-h-14 flex-col items-center justify-center gap-0.5 text-[11px] font-medium ${active ? "text-accent" : "text-muted hover:text-text"}`}>
                {t.icon}
                <span>{t.label}</span>
              </Link>
            </li>
          );
        })}
      </ul>
    </nav>
  );
}
