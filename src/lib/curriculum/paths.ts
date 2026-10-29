import type { Track } from "./types";

/** Learning paths, in the order they appear on the site. A track joins one with `path` in track.json and is ordered within it by `order`. */
export const PATHS = [
  { id: "start", title: "Start here", blurb: "No experience needed. Learn what a program is." },
  { id: "c", title: "C", blurb: "Memory, pointers and the data structures under every system." },
  { id: "rust", title: "Rust", blurb: "The same low-level control, with the compiler checking your memory use." },
  { id: "go", title: "Go", blurb: "A small, fast language with concurrency built in." },
  { id: "low", title: "Machine level", blurb: "Go below the language: assembly and a kernel of your own." },
  { id: "security", title: "Security", blurb: "Break it, fix it, and read the evidence: memory bugs, cryptography, packets and forensics." },
] as const;

export type PathId = (typeof PATHS)[number]["id"];

export const pathIndex = (id: string) => {
  const i = PATHS.findIndex((p) => p.id === id);
  return i < 0 ? PATHS.length : i;
};

export const compareTracks = (a: Track, b: Track) => pathIndex(a.path) - pathIndex(b.path) || a.order - b.order || a.slug.localeCompare(b.slug);

/** Tracks grouped by path, each group already ordered. Empty paths are dropped. */
export function groupByPath(tracks: Track[]) {
  return PATHS.map((p) => ({ ...p, tracks: tracks.filter((t) => t.path === p.id) })).filter((g) => g.tracks.length > 0);
}
