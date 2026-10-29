import type { Difficulty } from "../gamification";

export type Language = "c" | "rust" | "asm" | "go" | "kernel";

/** Display names and the editor's file name per language. */
export const LANGUAGE_LABEL: Record<Language, string> = { c: "C", rust: "Rust", asm: "x86-64 assembly", go: "Go", kernel: "C (bare-metal kernel)" };
export const LANGUAGE_FILE: Record<Language, string> = { c: "main.c", rust: "main.rs", asm: "starter.s", go: "main.go", kernel: "kernel.c" };

/** `set` rewrites the initial value of the named variables in the learner's source (`int minutes = 275;`), so a program that prints hardcoded text fails the case. */
export type StdoutCase = { name: string; stdin?: string; expected: string; set?: Record<string, string> };

export type Exercise = {
  /** Stable id, used as the progress key. Never change once published. */
  id: string;
  language: Language;
  /** Short task statement shown above the editor (markdown). */
  task: string;
  starter: string;
  /** Reference solution. Server-only: used by the curriculum check and never sent to the browser. */
  solution: string;
  /** "stdout": compare program output. "harness": hidden tests call the learner's functions. */
  mode: "stdout" | "harness";
  cases?: StdoutCase[];
  /** C: code using CHECK(name, cond, fmt, ...) with its own main(). Rust: #[test] functions. asm: C file (tests.c) declaring the learner's functions extern. Go: a bm_test.go file (package main, with its own imports) of TestXxx functions. */
  harness?: string;
  hints: string[];
};

export type Lesson = {
  slug: string;
  title: string;
  /** One sentence; doubles as the meta description. */
  summary: string;
  minutes: number;
  difficulty: Difficulty;
  /** "review" lessons consolidate earlier ones: no new concepts, familiar ideas combined in a new setting. */
  kind: "lesson" | "review";
  /** Lowercase kebab-case topic tags, from `tags:` in the frontmatter (comma-separated). */
  tags: string[];
  /** Lesson body (markdown). */
  body: string;
  exercise: Exercise;
  /** Optional harder variant, unlocked once the lesson is passed. Its exercise id is the lesson's plus "#challenge". */
  challenge?: { brief: string; exercise: Exercise };
};

export type Track = {
  slug: string;
  title: string;
  /** Meta-description-length summary. */
  summary: string;
  description: string;
  level: "Beginner" | "Intermediate" | "Advanced";
  language: Language;
  prerequisite?: string;
  /** Learning path this course belongs to (see paths.ts) and its position within it. */
  path: string;
  order: number;
  lessons: Lesson[];
};

/** What the browser is allowed to see of an exercise. */
export type PublicExercise = Pick<Exercise, "id" | "language" | "task" | "starter" | "hints" | "mode">;

export const toPublicExercise = ({ id, language, task, starter, hints, mode }: Exercise): PublicExercise => ({
  id,
  language,
  task,
  starter,
  hints,
  mode,
});
