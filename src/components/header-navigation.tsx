"use client";

import Link from "next/link";
import { usePathname } from "next/navigation";
import { useEffect, useRef, useState } from "react";
import { logout } from "@/app/(auth)/actions";
import { levelFor } from "@/lib/gamification";
import { ButtonLink } from "./ui";
import { ThemeToggle } from "./theme-toggle";

type NavigationUser = { name: string; username: string | null; xp: number; role: "user" | "admin" };

export function HeaderNavigation({ user }: { user: NavigationUser | null }) {
  const pathname = usePathname();
  const [open, setOpen] = useState(false);
  const panel = useRef<HTMLDivElement>(null);
  const trigger = useRef<HTMLButtonElement>(null);
  const links = [
    ...(user ? [{ href: "/dashboard", label: "Dashboard" }] : []),
    { href: "/learn", label: "Courses" },
    { href: "/leaderboard", label: "Leaderboard" },
  ];

  useEffect(() => {
    if (!open) return;
    const dismiss = (event: PointerEvent) => {
      if (!panel.current?.contains(event.target as Node)) setOpen(false);
    };
    const escape = (event: KeyboardEvent) => {
      if (event.key === "Escape") {
        setOpen(false);
        trigger.current?.focus();
      }
    };
    document.addEventListener("pointerdown", dismiss);
    document.addEventListener("keydown", escape);
    return () => {
      document.removeEventListener("pointerdown", dismiss);
      document.removeEventListener("keydown", escape);
    };
  }, [open]);

  const navLink = (href: string, label: string, mobile = false) => {
    const active = pathname === href || pathname.startsWith(`${href}/`);
    return <Link key={href} href={href} aria-current={active ? "page" : undefined} onClick={() => setOpen(false)} className={`${mobile ? "flex min-h-11 items-center" : "inline-flex min-h-16 items-center"} px-3 text-sm font-medium transition-colors ${mobile ? "rounded-lg" : "border-b-2 pt-0.5"} ${active ? (mobile ? "bg-accent-soft text-accent" : "border-accent text-text") : `${mobile ? "" : "border-transparent"} text-muted hover:text-text`}`}>{label}</Link>;
  };

  return (
    <div className="flex min-w-0 flex-1 items-center justify-end gap-3 lg:justify-between lg:gap-6">
      <nav aria-label="Main" className="hidden items-center gap-1 lg:flex">
        {links.map(({ href, label }) => navLink(href, label))}
      </nav>
      <div className="flex items-center gap-2">
        {!user && <div className="hidden items-center gap-2 lg:flex"><ThemeToggle /><ButtonLink href="/login" variant="ghost">Sign in</ButtonLink><ButtonLink href="/signup">Start learning</ButtonLink></div>}
        <div ref={panel} className={`relative ${!user ? "lg:hidden" : ""}`} onBlur={(event) => { if (!event.currentTarget.contains(event.relatedTarget)) setOpen(false); }}>
          <button ref={trigger} type="button" aria-expanded={open} aria-controls="header-account-panel" onClick={() => setOpen(!open)} className="inline-flex min-h-11 items-center gap-2.5 rounded-lg border border-line-strong px-3 text-sm font-medium hover:bg-surface-2">
            {user && <span aria-hidden="true" className="hidden h-7 w-7 items-center justify-center rounded-full bg-accent text-xs font-bold text-accent-ink lg:inline-flex">{user.name.charAt(0).toUpperCase()}</span>}
            <span className="lg:hidden">Menu</span>
            {user && <span className="hidden max-w-28 truncate lg:block">{user.name}</span>}
            <svg aria-hidden="true" viewBox="0 0 16 16" fill="none" stroke="currentColor" className={`h-4 w-4 text-muted transition-transform ${open ? "rotate-180" : ""}`}><path d="m4 6 4 4 4-4" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" /></svg>
          </button>
          {open && <div id="header-account-panel" className="absolute right-0 top-full z-50 mt-2 max-h-[calc(100dvh-6rem)] w-80 max-w-[calc(100vw-2rem)] overflow-y-auto panel p-2 shadow-xl shadow-black/15">
            <nav aria-label="Mobile navigation" className="mb-2 grid border-b border-border pb-2 lg:hidden">{links.map(({ href, label }) => navLink(href, label, true))}</nav>
            {user ? <>
              <UserCard name={user.name} xp={user.xp} />
              <nav aria-label="Account" className="grid">
                {user.username && navLink(`/u/${user.username}`, "Your profile", true)}
                {navLink("/settings", "Settings", true)}
                {user.role === "admin" && navLink("/admin", "Admin", true)}
              </nav>
            </> : <div className="grid gap-2 p-1"><ButtonLink href="/signup" onClick={() => setOpen(false)}>Start learning</ButtonLink><ButtonLink href="/login" variant="secondary" onClick={() => setOpen(false)}>Sign in</ButtonLink></div>}
            <div className="mt-2 flex items-center justify-between border-t border-border px-3 pt-2"><span className="text-sm text-muted">Appearance</span><ThemeToggle showLabel /></div>
            {user && <form action={logout} className="mt-2 border-t border-border pt-2"><button type="submit" className="flex min-h-11 w-full items-center rounded-lg px-3 text-sm text-muted hover:bg-surface-2 hover:text-text">Sign out</button></form>}
          </div>}
        </div>
      </div>
    </div>
  );
}

function UserCard({ name, xp }: { name: string; xp: number }) {
  const info = levelFor(xp);
  const percent = Math.round(info.progress * 100);
  return (
    <div className="mb-2 rounded-2xl border border-border bg-surface-2 p-4">
      <div className="flex items-center gap-4">
        <span aria-hidden="true" className="inline-flex h-12 w-12 shrink-0 items-center justify-center rounded-full bg-accent text-xl font-semibold text-accent-ink">{name.charAt(0).toUpperCase()}</span>
        <div className="min-w-0">
          <p className="truncate text-lg font-semibold leading-tight">{name}</p>
          <p className="mt-0.5 text-sm font-medium text-accent">Level {info.level}</p>
        </div>
      </div>
      <div className="mt-4 border-t border-border pt-4">
        {info.next === null ? (
          <p className="text-sm"><span className="font-semibold">{xp.toLocaleString()}</span> <span className="text-muted">XP · max level</span></p>
        ) : <>
          <div className="flex items-baseline justify-between gap-2 text-sm">
            <p><span className="font-semibold">{xp.toLocaleString()}</span> <span className="text-muted">/ {info.next.toLocaleString()} XP</span></p>
            <p className="font-semibold text-accent">{percent}%</p>
          </div>
          <div role="progressbar" aria-label={`Progress to level ${info.level + 1}`} aria-valuemin={0} aria-valuemax={100} aria-valuenow={percent} className="mt-2 h-2 overflow-hidden rounded-full bg-accent-soft">
            <div className="h-full rounded-full bg-gradient-to-r from-accent to-[color-mix(in_srgb,var(--accent),var(--hard)_55%)]" style={{ width: `${percent}%` }} />
          </div>
          <p className="mt-2 text-sm text-muted"><span className="font-semibold text-text">{(info.next - xp).toLocaleString()} XP</span> to Level {info.level + 1}</p>
        </>}
      </div>
    </div>
  );
}
