// Loads the curriculum from content/<track>/<NN-lesson>/ at startup.
// Server-only in practice (uses fs); scripts import it directly.
import { readdirSync, readFileSync, existsSync } from "node:fs";
import path from "node:path";
import { challengeIdOf, isChallengeId, isDifficulty, isPracticeId, practiceIdOf } from "../gamification";
import { compareTracks } from "./paths";
import { parsePractice } from "./practice";
import { parseLearningMetadata } from "./learning-metadata";
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

/** A C lesson may ship `driver.c` (out-of-process verdict) instead of `tests.c`; the driver wins if both exist. */
function harnessFiles(dir: string, mode: Exercise["mode"], language: Track["language"], harnessExt: string): Pick<Exercise, "harness" | "driver"> {
  if (mode !== "harness") return {};
  const driver = path.join(dir, "driver.c");
  if (language === "c" && existsSync(driver)) return { driver: read(driver) };
  return { harness: read(path.join(dir, `tests.${harnessExt}`)) };
}

function loadLesson(trackDir: string, dirName: string, language: Track["language"]): Lesson | null {
  const dir = path.join(trackDir, dirName);
  const ext = { c: "c", asm: "s", go: "go", rust: "rs", kernel: "c" }[language];
  const harnessExt = language === "asm" || language === "kernel" ? "c" : ext;
  const { meta, body } = parseFrontmatter(read(path.join(dir, "lesson.md")));
  // Explicit drafts may be incomplete. Published lessons still fail closed on
  // missing files, so an accidental content regression cannot disappear silently.
  if (meta.draft === "true") return null;
  const learningFile = path.join(dir, "learning.json");
  let learning: Lesson["learning"];
  if (existsSync(learningFile)) {
    let raw: unknown;
    try { raw = JSON.parse(read(learningFile)); }
    catch (error) { throw new Error(`${dirName}/learning.json is not valid JSON: ${(error as Error).message}`); }
    learning = parseLearningMetadata(raw, `${dirName}/learning.json`);
  }
  const mode = meta.mode === "stdout" ? "stdout" : "harness";
  const revision = meta.revision === undefined ? 1 : Number(meta.revision);
  if (!Number.isInteger(revision) || revision < 1) throw new Error(`Lesson ${dir}: revision must be a whole number from 1`);
  const exercise: Exercise = {
    id: meta.id,
    language,
    task: read(path.join(dir, "task.md")).trim(),
    starter: read(path.join(dir, `starter.${ext}`)),
    solution: read(path.join(dir, `solution.${ext}`)),
    mode,
    ...(revision > 1 ? { revision } : {}),
    cases: mode === "stdout" ? JSON.parse(read(path.join(dir, "cases.json"))) : undefined,
    ...harnessFiles(dir, mode, language, harnessExt),
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
  const practiceFile = path.join(dir, "practice.json");
  let practice: Lesson["practice"];
  if (existsSync(practiceFile)) {
    try {
      practice = parsePractice(JSON.parse(read(practiceFile)), dirName);
    } catch (e) {
      throw e instanceof SyntaxError ? new Error(`${dirName}/practice.json is not valid JSON: ${e.message}`) : e;
    }
  }
  const quizFile = path.join(dir, "quiz.json");
  let quiz: Lesson["quiz"];
  if (existsSync(quizFile)) {
    let raw: unknown;
    try {
      raw = JSON.parse(read(quizFile));
    } catch (e) {
      throw new Error(`${dirName}/quiz.json is not valid JSON: ${(e as Error).message}`);
    }
    if (!Array.isArray(raw) || raw.length < 1 || raw.length > 12) throw new Error(`${dirName}/quiz.json must be an array of 1-12 questions`);
    quiz = raw.map((q, i) => parsePractice(q, `${dirName}/quiz.json[${i}]`));
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
    ...(learning ? { learning } : {}),
    body: body.trim(),
    exercise,
    challenge,
    practice,
    quiz,
  };
}

function loadTracks(only?: string): Track[] {
  if (!existsSync(CONTENT_DIR)) return [];
  const present = readdirSync(CONTENT_DIR).filter((d) => (!only || d === only) && existsSync(path.join(CONTENT_DIR, d, "track.json")));
  return present.flatMap((slug) => {
    const dir = path.join(CONTENT_DIR, slug);
    const meta = JSON.parse(read(path.join(dir, "track.json")));
    if (meta.draft === true) return [];
    const lessonDirs = readdirSync(dir)
      .filter((d) => /^\d+-/.test(d))
      .sort();
    return [{ slug, ...meta, lessons: lessonDirs.map((d) => loadLesson(dir, d, meta.language)).filter((lesson) => lesson !== null) } as Track];
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

/** Finds the lesson behind a practice id (`<lesson id>#practice`); only lessons that have a practice.json match. */
export function findPractice(practiceId: string) {
  if (!isPracticeId(practiceId)) return undefined;
  for (const track of getTracks()) {
    for (const lesson of track.lessons) {
      if (lesson.practice && practiceIdOf(lesson.exercise.id) === practiceId) return { track, lesson, exercise: lesson.exercise, practice: lesson.practice };
    }
  }
  return undefined;
}

export { PRACTICE_KINDS, assembleParsons, canonicalSubmission, checkPractice, revealAnswer, fillBlanks, parsePractice, toPublicPractice } from "./practice";
export type { Practice, PracticeCheck, PracticeKind, PracticeSubmission, PublicPractice } from "./practice";
export { TYPE_FILTERS, TYPE_LABEL, isLessonType, lessonTypes, parseTypeFilter, trackTypeCounts, typeCounts } from "./lesson-types";
export type { LessonType, TypeCounts } from "./lesson-types";
export const lessonPath = (trackSlug: string, lessonSlug: string) => `/learn/${trackSlug}/${lessonSlug}`;
