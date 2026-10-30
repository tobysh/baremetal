"use server";

import { getTracks } from "@/lib/curriculum";

/** Compact, public course index: tracks as [slug, title], lessons as [trackIndex, slug, title]. */
export type PaletteIndex = { tracks: [string, string][]; lessons: [number, string, string][] };

/** Fetched once, the first time the palette opens, so no page carries it. */
export async function loadPaletteIndex(): Promise<PaletteIndex> {
  const tracks = getTracks();
  return {
    tracks: tracks.map((t) => [t.slug, t.title]),
    lessons: tracks.flatMap((t, i) => t.lessons.map((l): [number, string, string] => [i, l.slug, l.title])),
  };
}
