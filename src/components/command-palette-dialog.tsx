"use client";

import { useRouter } from "next/navigation";
import { useEffect, useId, useMemo, useRef, useState } from "react";
import { loadPaletteIndex, type PaletteIndex } from "./command-palette-actions";

type Item = { href: string; label: string; hint: string; haystack: string };

/** Higher is better; -1 means no match. Substring beats an in-order subsequence. */
function score(query: string, text: string): number {
  const at = text.indexOf(query);
  if (at >= 0) return 1000 - at - text.length / 100;
  let i = 0;
  let gaps = 0;
  let last = -1;
  for (const ch of query) {
    const next = text.indexOf(ch, i);
    if (next < 0) return -1;
    if (last >= 0) gaps += next - last - 1;
    last = next;
    i = next + 1;
  }
  return Math.max(1, 500 - gaps * 10);
}

export default function PaletteDialog({ open, onClose, signedIn, username }: { open: boolean; onClose: () => void; signedIn: boolean; username: string | null }) {
  const router = useRouter();
  const dialog = useRef<HTMLDialogElement>(null);
  const input = useRef<HTMLInputElement>(null);
  const list = useRef<HTMLUListElement>(null);
  const id = useId();
  const [query, setQuery] = useState("");
  const [active, setActive] = useState(0);
  const [index, setIndex] = useState<PaletteIndex | null>(null);
  const [failed, setFailed] = useState(false);
  const requested = useRef(false);

  useEffect(() => {
    const el = dialog.current;
    if (!el) return;
    if (open && !el.open) {
      el.showModal(); // native modal dialog: focus trap, inert background and Escape
      input.current?.focus();
    } else if (!open && el.open) el.close();
    if (open && !requested.current) {
      requested.current = true;
      loadPaletteIndex().then(setIndex, () => { setFailed(true); requested.current = false; });
    }
  }, [open]);

  const items = useMemo<Item[]>(() => {
    const pages: Item[] = [
      ...(signedIn ? [{ href: "/dashboard", label: "Dashboard", hint: "Page" }] : []),
      { href: "/learn", label: "Courses", hint: "Page" },
      { href: "/shell", label: "Shell", hint: "Page" },
      { href: "/leaderboard", label: "Leaderboard", hint: "Page" },
      ...(signedIn ? [{ href: "/settings", label: "Settings", hint: "Page" }] : []),
      ...(signedIn && username ? [{ href: `/u/${username}`, label: "Your profile", hint: "Page" }] : []),
    ].map((p) => ({ ...p, haystack: p.label.toLowerCase() }));
    if (!index) return pages;
    const tracks = index.tracks.map(([slug, title]): Item => ({ href: `/learn/${slug}`, label: title, hint: "Course", haystack: `${title} ${slug}`.toLowerCase() }));
    const lessons = index.lessons.map(([t, slug, title]): Item => ({ href: `/learn/${index.tracks[t][0]}/${slug}`, label: title, hint: index.tracks[t][1], haystack: `${title} ${index.tracks[t][1]}`.toLowerCase() }));
    return [...pages, ...tracks, ...lessons];
  }, [index, signedIn, username]);

  const results = useMemo(() => {
    const q = query.trim().toLowerCase();
    if (!q) return items.slice(0, 8);
    return items
      .map((item) => ({ item, s: score(q, item.haystack) }))
      .filter((r) => r.s > 0)
      .sort((a, b) => b.s - a.s)
      .slice(0, 12)
      .map((r) => r.item);
  }, [items, query]);

  useEffect(() => {
    list.current?.querySelector('[aria-selected="true"]')?.scrollIntoView({ block: "nearest" });
  }, [active, results]);

  const go = (item: Item | undefined) => {
    if (!item) return;
    onClose();
    router.push(item.href);
  };

  const onKeyDown = (event: React.KeyboardEvent) => {
    if (event.key === "ArrowDown") { event.preventDefault(); setActive((a) => (results.length ? (a + 1) % results.length : 0)); }
    else if (event.key === "ArrowUp") { event.preventDefault(); setActive((a) => (results.length ? (a - 1 + results.length) % results.length : 0)); }
    else if (event.key === "Home") { event.preventDefault(); setActive(0); }
    else if (event.key === "End") { event.preventDefault(); setActive(Math.max(0, results.length - 1)); }
    else if (event.key === "Enter") { event.preventDefault(); go(results[active]); }
  };

  return (
    <dialog
      ref={dialog}
      aria-label="Search"
      onClose={() => { setQuery(""); setActive(0); onClose(); }}
      onClick={(event) => { if (event.target === dialog.current) onClose(); }}
      className="m-auto mt-[12vh] w-[min(36rem,calc(100vw-2rem))] rounded-2xl border border-border bg-surface p-0 text-text shadow-xl shadow-black/25 backdrop:bg-black/50"
    >
      <div role="search" onKeyDown={onKeyDown}>
        <input
          ref={input}
          type="text"
          role="combobox"
          aria-expanded="true"
          aria-controls={`${id}-list`}
          aria-activedescendant={results[active] ? `${id}-${active}` : undefined}
          aria-autocomplete="list"
          aria-label="Search lessons, courses and pages"
          placeholder="Search lessons, courses and pages"
          autoComplete="off"
          spellCheck={false}
          value={query}
          onChange={(event) => { setQuery(event.target.value); setActive(0); }}
          className="w-full rounded-t-2xl border-b border-border bg-transparent px-4 py-3.5 text-base outline-none placeholder:text-muted focus-visible:ring-2 focus-visible:ring-inset focus-visible:ring-accent"
        />
        <ul ref={list} id={`${id}-list`} role="listbox" aria-label="Results" className="max-h-[50vh] overflow-y-auto p-2">
          {results.map((item, i) => (
            <li
              key={item.href}
              id={`${id}-${i}`}
              role="option"
              aria-selected={i === active}
              onPointerMove={() => setActive(i)}
              onClick={() => go(item)}
              className={`flex min-h-11 cursor-pointer items-center justify-between gap-3 rounded-lg px-3 text-sm ${i === active ? "bg-accent-soft text-accent" : ""}`}
            >
              <span className="truncate">{item.label}</span>
              <span className="shrink-0 truncate text-xs text-muted">{item.hint}</span>
            </li>
          ))}
        </ul>
        <p role="status" className="border-t border-border px-4 py-2 text-xs text-muted">
          {failed ? "Could not load lessons; pages only." : results.length === 0 ? "No matches." : !index ? "Loading lessons…" : `${results.length} result${results.length === 1 ? "" : "s"}. Arrow keys to move, Enter to open, Esc to close.`}
        </p>
      </div>
    </dialog>
  );
}
