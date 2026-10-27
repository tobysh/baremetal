"use client";

import { useSyncExternalStore } from "react";

export type ThemeChoice = "system" | "light" | "dark";
const KEY = "bm:theme";
const EVENT = "bm:theme-changed";
const ORDER: ThemeChoice[] = ["system", "light", "dark"];
const LABEL: Record<ThemeChoice, string> = { system: "System theme", light: "Light theme", dark: "Dark theme" };

/**
 * Runs before first paint (inlined in <head>) so the page never flashes the
 * wrong theme. The choice is a per-device preference kept in localStorage.
 */
export const themeInitScript = `try{var t=localStorage.getItem("${KEY}");if(t==="light"||t==="dark")document.documentElement.dataset.theme=t}catch(e){}`;

function read(): ThemeChoice {
  try {
    const t = localStorage.getItem(KEY);
    return t === "light" || t === "dark" ? t : "system";
  } catch {
    return "system";
  }
}

function subscribe(cb: () => void) {
  window.addEventListener(EVENT, cb);
  window.addEventListener("storage", cb);
  return () => {
    window.removeEventListener(EVENT, cb);
    window.removeEventListener("storage", cb);
  };
}

function apply(choice: ThemeChoice) {
  try {
    if (choice === "system") localStorage.removeItem(KEY);
    else localStorage.setItem(KEY, choice);
  } catch {}
  if (choice === "system") delete document.documentElement.dataset.theme;
  else document.documentElement.dataset.theme = choice;
  window.dispatchEvent(new Event(EVENT));
}

export function ThemeToggle({ showLabel = false }: { showLabel?: boolean }) {
  const choice = useSyncExternalStore(subscribe, read, () => "system" as ThemeChoice);
  const next = ORDER[(ORDER.indexOf(choice) + 1) % ORDER.length];
  return (
    <button
      type="button"
      onClick={() => apply(next)}
      aria-label={`${LABEL[choice]}. Switch to ${LABEL[next].toLowerCase()}`}
      title={`${LABEL[choice]} — click for ${LABEL[next].toLowerCase()}`}
      className={`inline-flex min-h-11 items-center justify-center gap-2 rounded-lg text-muted hover:bg-surface-2 hover:text-text ${showLabel ? "px-2 text-xs" : "w-11"}`}
    >
      <svg viewBox="0 0 24 24" className="h-[18px] w-[18px]" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
        {choice === "light" ? (
          <>
            <circle cx="12" cy="12" r="4" />
            <path d="M12 2v2M12 20v2M4.93 4.93l1.41 1.41M17.66 17.66l1.41 1.41M2 12h2M20 12h2M4.93 19.07l1.41-1.41M17.66 6.34l1.41-1.41" />
          </>
        ) : choice === "dark" ? (
          <path d="M21 12.8A9 9 0 1 1 11.2 3a7 7 0 0 0 9.8 9.8Z" />
        ) : (
          <>
            <rect x="3" y="4" width="18" height="12" rx="2" />
            <path d="M8 20h8M12 16v4" />
          </>
        )}
      </svg>
      {showLabel && <span>{choice.charAt(0).toUpperCase() + choice.slice(1)}</span>}
    </button>
  );
}
