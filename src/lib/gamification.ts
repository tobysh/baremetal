// XP, levels and difficulty. Shared by server and browser.

export const DIFFICULTIES = {
  easy: { label: "Easy", xp: 5 },
  medium: { label: "Medium", xp: 10 },
  hard: { label: "Hard", xp: 20 },
  impossible: { label: "Impossible", xp: 40 },
} as const;
export type Difficulty = keyof typeof DIFFICULTIES;
export const isDifficulty = (v: unknown): v is Difficulty => typeof v === "string" && v in DIFFICULTIES;
export const xpFor = (d: Difficulty) => DIFFICULTIES[d].xp;
/** A lesson's optional challenge is worth half its XP, rounded up (easy 3, medium 5, hard 10, impossible 20). */
export const challengeXpFor = (d: Difficulty) => Math.ceil(DIFFICULTIES[d].xp / 2);
/** Challenges are stored as exercises with this suffix on the lesson's id. */
export const CHALLENGE_SUFFIX = "#challenge";
export const isChallengeId = (id: string) => id.endsWith(CHALLENGE_SUFFIX);
export const challengeIdOf = (lessonExerciseId: string) => lessonExerciseId + CHALLENGE_SUFFIX;

// Cumulative XP needed to reach each level (index 0 is level 1). Levels 2 and 3 come quickly so a new learner feels
// movement; after that each level costs noticeably more than the last: 8, 24, 60, 120, 210, 330, 500, 720, 1000.
// Level 10 is the cap, and XP keeps counting past it. Changing this table changes everyone's level, never their XP.
const LEVEL_FLOORS = [0, 8, 24, 60, 120, 210, 330, 500, 720, 1000];
const xpToReach = (level: number) => LEVEL_FLOORS[level - 1] ?? Infinity;

// Medieval ranks, all respectable: everyone starts as a Novice.
const TITLES = ["Novice", "Apprentice", "Squire", "Scribe", "Knight", "Ranger", "Paladin", "Sorcerer", "Oracle", "Archmage"];
const TITLE_BLURBS = [
  "Everyone starts here. Your first program is a few minutes away.",
  "You can print, store values and make simple decisions.",
  "You can hold your own with loops and functions.",
  "You write careful, readable code and can read someone else's.",
  "You understand how memory is laid out and handle pointers with care.",
  "You build real data structures and know when to reach for each.",
  "You are at ease with the machine: bits, bytes and the borrow checker.",
  "You combine ideas from several languages and lean on the standard tools.",
  "You can reason about code at every level, from source to silicon.",
  "You have worked through the whole road. Challenges still earn XP.",
];
export const titleBlurb = (level: number) => TITLE_BLURBS[Math.min(level, TITLE_BLURBS.length) - 1];

export type LevelInfo = {
  level: number;
  title: string;
  xp: number;
  /** XP at which this level started. */
  floor: number;
  /** XP needed for the next level, or null at the top. */
  next: number | null;
  /** 0–1 progress through the current level. */
  progress: number;
};

export function levelFor(xp: number): LevelInfo {
  let level = 1;
  while (xpToReach(level + 1) <= xp) level++;
  const floor = xpToReach(level);
  const next = level < TITLES.length ? xpToReach(level + 1) : null;
  return {
    level,
    title: TITLES[Math.min(level, TITLES.length) - 1],
    xp,
    floor,
    next,
    progress: next === null ? 1 : (xp - floor) / (next - floor),
  };
}

// ---------- Usernames ----------

export const USERNAME_RE = /^[a-z0-9](?:[a-z0-9_-]{1,18}[a-z0-9])?$/;
const RESERVED = new Set([
  "admin", "administrator", "root", "support", "help", "api", "settings", "dashboard", "login", "signup", "learn",
  "leaderboard", "privacy", "terms", "cookies", "baremetal", "moderator", "staff", "system", "null", "undefined", "me", "you",
]);

/** Returns an error message, or null if the username is acceptable. Expects a lowercased value. */
export function usernameProblem(name: string): string | null {
  if (name.length < 3 || name.length > 20) return "Usernames are 3–20 characters";
  if (!USERNAME_RE.test(name)) return "Use lowercase letters, numbers, - and _ (starting and ending with a letter or number)";
  if (RESERVED.has(name)) return "That username is reserved";
  return null;
}
