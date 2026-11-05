"use server";

import { getTracks } from "@/lib/curriculum";
import { PATHS } from "@/lib/curriculum/paths";
import { LANGUAGE_LABEL } from "@/lib/curriculum/types";

/**
 * Compact, public course index, fetched once when the palette first opens.
 * tracks: [slug, title, language, level, outcomes and summary, path id]
 * lessons: [trackIndex, slug, title, summary, tags]
 * paths: [id, title, blurb]
 * Only authored metadata is indexed (titles, summaries, tags, outcomes); lesson bodies are not searched.
 */
export type PaletteIndex = {
  tracks: [string, string, string, string, string, string][];
  lessons: [number, string, string, string, string][];
  paths: [string, string, string][];
};

export async function loadPaletteIndex(): Promise<PaletteIndex> {
  const tracks = getTracks();
  return {
    tracks: tracks.map((t) => [t.slug, t.title, LANGUAGE_LABEL[t.language], t.level, [...t.outcomes, t.summary].join(" "), t.path]),
    lessons: tracks.flatMap((t, i) => t.lessons.map((l): [number, string, string, string, string] => [i, l.slug, l.title, l.summary, l.tags.join(" ")])),
    paths: PATHS.map((p) => [p.id, p.title, p.blurb]),
  };
}
