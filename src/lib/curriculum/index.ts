// Loads the curriculum from content/<track>/<NN-lesson>/ at startup.
// Server-only in practice (uses fs); scripts import it directly.
import { readdirSync, readFileSync, existsSync } from "node:fs";
import path from "node:path";
import { challengeIdOf, isChallengeId, isDifficulty } from "../gamification";
import { compareTracks } from "./paths";
import type { Exercise, Lesson, Track } from "./types";

const CONTENT_DIR = path.join(process.cwd(), "content");

function parseFrontmatter(src: string): { meta: Record<string, string>; body: string } {
  const m = src.match(/^---\n([\s\S]*?)\n---\n([\s\S]*)$/);
  if (!m) return { meta: {}, body: src };
  const meta: Record<string, string> = {};
  for (const line of m[1].split("\n")) {
    const i = line.indexOf(":");
    if (i > 0) {
      let v = line.slice(i + 1).trim();
      // Values may be quoted (needed in YAML when they contain a colon); strip one matching pair.
      if (v.length >= 2 && (v[0] === '"' || v[0] === "'") && v[v.length - 1] === v[0]) v = v.slice(1, -1);
      meta[line.slice(0, i).trim()] = v;
    }
  }
  return { meta, body: m[2] };
}

const read = (p: string) => readFileSync(p, "utf8");

function loadLesson(trackDir: string, dirName: string, language: Track["language"]): Lesson {
  const dir = path.join(trackDir, dirName);
  const ext = { c: "c", asm: "s", go: "go", rust: "rs", kernel: "c" }[language];
  const harnessExt = language === "asm" || language === "kernel" ? "c" : ext;
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
    harness: mode === "harness" ? read(path.join(dir, `tests.${harnessExt}`)) : undefined,
    hints: read(path.join(dir, "hints.md"))
      .split(/\n===\n/)
      .map((h) => h.trim())
      .filter(Boolean),
  };
  const challengeFile = path.join(dir, "challenge.md");
  let challenge: Lesson["challenge"];
  if (existsSync(challengeFile)) {
    const hasTests = existsSync(path.join(dir, `challenge-tests.${harnessExt}`));
    const brief = read(challengeFile).trim();
    challenge = {
      brief,
      exercise: {
        id: challengeIdOf(meta.id),
        language,
        task: brief,
        starter: read(path.join(dir, `challenge-starter.${ext}`)),
        solution: read(path.join(dir, `challenge-solution.${ext}`)),
        mode: hasTests ? "harness" : "stdout",
        cases: hasTests ? undefined : JSON.parse(read(path.join(dir, "challenge-cases.json"))),
        harness: hasTests ? read(path.join(dir, `challenge-tests.${harnessExt}`)) : undefined,
        hints: [],
      },
    };
  }
  if (!meta.title || !meta.summary || !meta.id) throw new Error(`Lesson ${dir} is missing title, summary or id`);
  if (!isDifficulty(meta.difficulty)) throw new Error(`Lesson ${dir} needs difficulty: easy | medium | hard | impossible`);
  if (meta.kind && meta.kind !== "lesson" && meta.kind !== "review") throw new Error(`Lesson ${dir}: kind must be lesson or review`);
  const tags = (meta.tags ?? "").split(",").map((t) => t.trim()).filter(Boolean);
  for (const t of tags) if (!/^[a-z0-9]+(-[a-z0-9]+)*$/.test(t)) throw new Error(`Lesson ${dir}: tag "${t}" must be lowercase kebab-case`);
  return {
    slug: dirName.replace(/^\d+-/, ""),
    title: meta.title,
    summary: meta.summary,
    minutes: Number(meta.minutes ?? 15),
    difficulty: meta.difficulty,
    kind: meta.kind === "review" ? "review" : "lesson",
    tags,
    body: body.trim(),
    exercise,
    challenge,
  };
}

function loadTracks(only?: string): Track[] {
  if (!existsSync(CONTENT_DIR)) return [];
  const present = readdirSync(CONTENT_DIR).filter((d) => (!only || d === only) && existsSync(path.join(CONTENT_DIR, d, "track.json")));
  return present.map((slug) => {
    const dir = path.join(CONTENT_DIR, slug);
    const meta = JSON.parse(read(path.join(dir, "track.json")));
    const lessonDirs = readdirSync(dir)
      .filter((d) => /^\d+-/.test(d))
      .sort();
    return { slug, ...meta, lessons: lessonDirs.map((d) => loadLesson(dir, d, meta.language)) } as Track;
  }).sort(compareTracks);
}

const globalForContent = globalThis as unknown as { __tracks?: Track[]; __tracksAt?: number };
const DEV_TTL_MS = 1000;
// In dev, reload at most once a second so edits to content/ show up without a restart,
// while a page that calls this dozens of times per render pays for one load.
export function getTracks(only?: string): Track[] {
  if (only) return loadTracks(only);
  if (process.env.NODE_ENV !== "production") {
    const now = performance.now();
    if (!globalForContent.__tracks || now - (globalForContent.__tracksAt ?? 0) > DEV_TTL_MS) {
      globalForContent.__tracks = loadTracks();
      globalForContent.__tracksAt = now;
    }
    return globalForContent.__tracks;
  }
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

/** Finds a lesson's exercise, or its challenge when the id ends in "#challenge". */
export function findExercise(exerciseId: string) {
  const wantChallenge = isChallengeId(exerciseId);
  for (const track of getTracks()) {
    for (const lesson of track.lessons) {
      if (!wantChallenge && lesson.exercise.id === exerciseId) return { track, lesson, exercise: lesson.exercise, isChallenge: false };
      if (wantChallenge && lesson.challenge?.exercise.id === exerciseId) return { track, lesson, exercise: lesson.challenge.exercise, isChallenge: true };
    }
  }
  return undefined;
}

export const lessonPath = (trackSlug: string, lessonSlug: string) => `/learn/${trackSlug}/${lessonSlug}`;
