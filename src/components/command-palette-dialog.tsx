"use client";

import Link from "next/link";
import { useRouter } from "next/navigation";
import { useEffect, useId, useMemo, useRef, useState } from "react";
import { loadPaletteIndex, type PaletteIndex } from "./command-palette-actions";

type Kind = "Page" | "Learning path" | "Course" | "Lesson";
/** `strong` is matched at full weight (title, tags); `weak` (summary, outcomes) at reduced weight. `rank` prefers beginner entry points on ties. */
type Item = { href: string; label: string; kind: Kind; context: string; strong: string; weak: string; rank: number };
const LEVEL_RANK: Record<string, number> = { Beginner: 0, Intermediate: 1, Advanced: 2 };

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
    const page = (href: string, label: string): Item => ({ href, label, kind: "Page", context: "", strong: label.toLowerCase(), weak: "", rank: 0 });
    const pages: Item[] = [
      ...(signedIn ? [page("/dashboard", "Dashboard")] : []),
      page("/learn", "Courses"),
      page("/shell", "Shell"),
      page("/leaderboard", "Leaderboard"),
      ...(signedIn ? [page("/settings", "Settings")] : []),
      ...(signedIn && username ? [page(`/u/${username}`, "Your profile")] : []),
    ];
    if (!index) return pages;
    const paths = index.paths.map(([id, title, blurb]): Item => ({ href: `/learn#path-${id}`, label: title, kind: "Learning path", context: blurb, strong: title.toLowerCase(), weak: blurb.toLowerCase(), rank: 0 }));
    const tracks = index.tracks.map(([slug, title, language, level, about]): Item => ({
      href: `/learn/${slug}`, label: title, kind: "Course", context: `${language} · ${level}`,
      strong: `${title} ${slug} ${language}`.toLowerCase(), weak: about.toLowerCase(), rank: LEVEL_RANK[level] ?? 1,
    }));
    const lessons = index.lessons.map(([t, slug, title, summary, tags]): Item => ({
      href: `/learn/${index.tracks[t][0]}/${slug}`, label: title, kind: "Lesson", context: `${index.tracks[t][1]} · ${summary}`,
      strong: `${title} ${tags.replace(/-/g, " ")} ${tags}`.toLowerCase(), weak: `${summary} ${index.tracks[t][1]} ${index.tracks[t][2]}`.toLowerCase(), rank: LEVEL_RANK[index.tracks[t][3]] ?? 1,
    }));
    return [...pages, ...paths, ...tracks, ...lessons];
  }, [index, signedIn, username]);

  const results = useMemo(() => {
    const words = query.trim().toLowerCase().split(/\s+/).filter(Boolean);
    if (!words.length) return items.filter((i) => i.kind === "Page" || i.kind === "Learning path").slice(0, 8);
    const kindWeight: Record<Kind, number> = { Page: 40, "Learning path": 30, Course: 20, Lesson: 0 };
    return items
      .map((item) => {
        // Every word must match somewhere; title and tag hits count fully, summary and outcome hits only substring-wise.
        let total = 0;
        for (const w of words) {
          const s = score(w, item.strong);
          const weak = item.weak.includes(w) ? 150 : -1;
          const best = Math.max(s, weak);
          if (best < 0) return { item, s: -1 };
          total += best;
        }
        return { item, s: total / words.length + kindWeight[item.kind] - item.rank * 15 };
      })
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
          aria-label="Search courses, lessons and pages"
          placeholder="Search a topic, language or lesson"
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
              className={`flex min-h-11 cursor-pointer items-center py-1.5 justify-between gap-3 rounded-lg px-3 text-sm ${i === active ? "bg-accent-soft text-accent" : ""}`}
            >
              <span className="min-w-0">
                <span className="block truncate">{item.label}</span>
                {item.context && <span className={`block truncate text-xs ${i === active ? "text-text-2" : "text-muted"}`}>{item.context}</span>}
              </span>
              <span className="shrink-0 rounded border border-border px-1.5 py-0.5 text-xs text-muted">{item.kind}</span>
            </li>
          ))}
        </ul>
        {results.length === 0 && index && (
          <p className="px-4 pb-3 text-sm text-text-2">
            Nothing matches “{query.trim()}”. Try a topic such as “pointers” or a language such as “Rust”, or{" "}
            <Link href="/learn" onClick={onClose} className="text-accent underline underline-offset-2">browse all courses</Link>.
          </p>
        )}
        <p role="status" className="border-t border-border px-4 py-2 text-xs text-muted">
          {failed ? "Could not load lessons; showing pages only." : !index ? "Loading lessons…" : results.length === 0 ? "No results." : `${results.length} result${results.length === 1 ? "" : "s"}. Searches titles, summaries, tags and course outcomes, not lesson text. Esc closes.`}
        </p>
      </div>
    </dialog>
  );
}
