// The Codex: named achievements. They are recognition only and never award XP, so the leaderboard stays
// about learning. Adding one is a single entry here plus `npm run backfill-achievements`.
import type { Difficulty } from "./gamification";
import { levelFor } from "./gamification";
import type { Rhythm } from "./rhythm";

export type CodexTrack = {
  slug: string;
  language: string;
  lessons: { id: string; difficulty: Difficulty; challenge: boolean }[];
};

export type PassInfo = { passedAt: Date; attemptsAtPass: number; unaided: boolean };

export type CodexContext = {
  tracks: CodexTrack[];
  /** Passed lessons by exercise id. */
  passed: Map<string, PassInfo>;
  /** Lesson exercise ids whose challenge is passed. */
  challenges: Set<string>;
  xp: number;
  rhythm: Rhythm;
  /** Successful reviews in total, and the lessons reviewed at least once. */
  reviewsPassed: number;
  reviewed: Set<string>;
};

export type Achievement = {
  id: string;
  name: string;
  description: string;
  /** Secret ones show "A hidden achievement" until earned. */
  secret?: boolean;
  check: (c: CodexContext) => boolean;
};

const passedLessons = (c: CodexContext) => c.tracks.flatMap((t) => t.lessons.map((l) => ({ ...l, track: t }))).filter((l) => c.passed.has(l.id));
const finished = (c: CodexContext, slug: string) => {
  const t = c.tracks.find((x) => x.slug === slug);
  return !!t && t.lessons.length > 0 && t.lessons.every((l) => c.passed.has(l.id));
};
const unaidedCount = (c: CodexContext) => [...c.passed.values()].filter((p) => p.unaided).length;
const hasLanguage = (c: CodexContext, language: string) => passedLessons(c).some((l) => l.track.language === language);

export const ACHIEVEMENTS: Achievement[] = [
  { id: "first-pass", name: "First Light", description: "Pass any lesson.", check: (c) => c.passed.size >= 1 },
  {
    id: "first-try",
    name: "Clean Strike",
    description: "Pass a medium or harder lesson on your first test run.",
    check: (c) => passedLessons(c).some((l) => l.difficulty !== "easy" && c.passed.get(l.id)!.attemptsAtPass === 1),
  },
  {
    id: "persevere",
    name: "Siege Breaker",
    description: "Pass a lesson after ten or more failed runs.",
    secret: true,
    check: (c) => [...c.passed.values()].some((p) => p.attemptsAtPass >= 11),
  },
  { id: "unaided-5", name: "Self-Taught", description: "Earn five Unaided seals.", check: (c) => unaidedCount(c) >= 5 },
  { id: "unaided-25", name: "Own Compass", description: "Earn twenty-five Unaided seals.", check: (c) => unaidedCount(c) >= 25 },
  { id: "track-first-steps", name: "First Steps Taken", description: "Finish Programming from Zero.", check: (c) => finished(c, "first-steps") },
  { id: "track-c-memory", name: "Keeper of Pointers", description: "Finish Pointers & Memory in C.", check: (c) => finished(c, "c-memory") },
  { id: "track-rust-foundations", name: "Friend of the Borrow Checker", description: "Finish Rust Foundations.", check: (c) => finished(c, "rust-foundations") },
  { id: "track-rust-http", name: "Gatekeeper", description: "Finish Build an HTTP Server in Rust.", check: (c) => finished(c, "rust-http") },
  { id: "polyglot", name: "Two Tongues", description: "Pass a lesson in C and one in Rust.", check: (c) => hasLanguage(c, "c") && hasLanguage(c, "rust") },
  {
    id: "impossible",
    name: "The Impossible Made Possible",
    description: "Pass an Impossible lesson.",
    check: (c) => passedLessons(c).some((l) => l.difficulty === "impossible"),
  },
  {
    id: "all-impossible",
    name: "Legend",
    description: "Pass every Impossible lesson.",
    check: (c) => {
      const all = c.tracks.flatMap((t) => t.lessons).filter((l) => l.difficulty === "impossible");
      return all.length > 0 && all.every((l) => c.passed.has(l.id));
    },
  },
  { id: "challenge-1", name: "Side Quest", description: "Pass any lesson challenge.", check: (c) => c.challenges.size >= 1 },
  {
    id: "challenge-track",
    name: "Completionist",
    description: "Pass every challenge in one course.",
    check: (c) => c.tracks.some((t) => t.lessons.some((l) => l.challenge) && t.lessons.filter((l) => l.challenge).every((l) => c.challenges.has(l.id))),
  },
  { id: "weekly-4", name: "Steady Hand", description: "Meet your weekly goal four weeks running.", check: (c) => c.rhythm.best >= 4 },
  { id: "rhythm-8", name: "Even Keel", description: "Meet your weekly goal eight weeks running.", check: (c) => c.rhythm.best >= 8 },
  { id: "review-1", name: "Second Look", description: "Pass your first review.", check: (c) => c.reviewsPassed >= 1 },
  { id: "review-10", name: "Scholar's Habit", description: "Pass ten reviews.", check: (c) => c.reviewsPassed >= 10 },
  {
    id: "revisit-track",
    name: "Retracing Steps",
    description: "Review every lesson in one course.",
    check: (c) => c.tracks.some((t) => t.lessons.length > 0 && t.lessons.every((l) => c.reviewed.has(l.id))),
  },
  { id: "archmage", name: "Archmage", description: "Reach level 10.", check: (c) => levelFor(c.xp).level >= 10 },
];

export const achievementById = new Map(ACHIEVEMENTS.map((a) => [a.id, a]));
