// Loads the curriculum from content/<track>/<NN-lesson>/ at startup.
// Server-only in practice (uses fs); scripts import it directly.
import { readdirSync, readFileSync, existsSync } from "node:fs";
import path from "node:path";
import { isDifficulty } from "../gamification";
import type { Exercise, Lesson, Track } from "./types";

const CONTENT_DIR = path.join(process.cwd(), "content");

/** Track order on the site. */
const TRACK_ORDER = ["first-steps", "c-memory", "rust-foundations", "rust-http"];

function parseFrontmatter(src: string): { meta: Record<string, string>; body: string } {
  const m = src.match(/^---\n([\s\S]*?)\n---\n([\s\S]*)$/);
  if (!m) return { meta: {}, body: src };
  const meta: Record<string, string> = {};
  for (const line of m[1].split("\n")) {
    const i = line.indexOf(":");
    if (i > 0) meta[line.slice(0, i).trim()] = line.slice(i + 1).trim();
  }
  return { meta, body: m[2] };
}

const read = (p: string) => readFileSync(p, "utf8");

function loadLesson(trackDir: string, dirName: string, language: Track["language"]): Lesson {
  const dir = path.join(trackDir, dirName);
  const ext = language === "c" ? "c" : "rs";
  const { meta, body } = parseFrontmatter(read(path.join(dir, "lesson.md")));
  const mode = meta.mode === "stdout" ? "stdout" : "harness";
  const exercise: Exercise = {
    id: meta.id,
    language,
    task: read(path.join(dir, "task.md")).trim(),
    starter: read(path.join(dir, `starter.${ext}`)),
    solution: read(path.join(dir, `solution.${ext}`)),
    mode,
    cases: mode === "stdout" ? JSON.parse(read(path.join(dir, "cases.json"))) : undefined,
    harness: mode === "harness" ? read(path.join(dir, `tests.${ext}`)) : undefined,
    hints: read(path.join(dir, "hints.md"))
      .split(/\n===\n/)
      .map((h) => h.trim())
      .filter(Boolean),
  };
  if (!meta.title || !meta.summary || !meta.id) throw new Error(`Lesson ${dir} is missing title, summary or id`);
  if (!isDifficulty(meta.difficulty)) throw new Error(`Lesson ${dir} needs difficulty: easy | medium | hard | impossible`);
  return {
    slug: dirName.replace(/^\d+-/, ""),
    title: meta.title,
    summary: meta.summary,
    minutes: Number(meta.minutes ?? 15),
    difficulty: meta.difficulty,
    body: body.trim(),
    exercise,
  };
}

function loadTracks(): Track[] {
  if (!existsSync(CONTENT_DIR)) return [];
  const present = readdirSync(CONTENT_DIR).filter((d) => existsSync(path.join(CONTENT_DIR, d, "track.json")));
  const ordered = [...TRACK_ORDER.filter((t) => present.includes(t)), ...present.filter((t) => !TRACK_ORDER.includes(t)).sort()];
  return ordered.map((slug) => {
    const dir = path.join(CONTENT_DIR, slug);
    const meta = JSON.parse(read(path.join(dir, "track.json")));
    const lessonDirs = readdirSync(dir)
      .filter((d) => /^\d+-/.test(d))
      .sort();
    return { slug, ...meta, lessons: lessonDirs.map((d) => loadLesson(dir, d, meta.language)) } as Track;
  });
}

const globalForContent = globalThis as unknown as { __tracks?: Track[] };
// Reload on every call in dev so edits to content/ show up without a restart.
export function getTracks(): Track[] {
  if (process.env.NODE_ENV !== "production") return loadTracks();
  return (globalForContent.__tracks ??= loadTracks());
}

export function getTrack(slug: string): Track | undefined {
  return getTracks().find((t) => t.slug === slug);
}

export function getLesson(trackSlug: string, lessonSlug: string) {
  const track = getTrack(trackSlug);
  if (!track) return undefined;
  const index = track.lessons.findIndex((l) => l.slug === lessonSlug);
  if (index < 0) return undefined;
  return { track, lesson: track.lessons[index], index, prev: track.lessons[index - 1], next: track.lessons[index + 1] };
}

export function findExercise(exerciseId: string) {
  for (const track of getTracks()) {
    for (const lesson of track.lessons) {
      if (lesson.exercise.id === exerciseId) return { track, lesson, exercise: lesson.exercise };
    }
  }
  return undefined;
}

export const lessonPath = (trackSlug: string, lessonSlug: string) => `/learn/${trackSlug}/${lessonSlug}`;
