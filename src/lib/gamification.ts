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

// Cumulative XP needed to reach level L is 4·L·(L−1): 8, 24, 48, 80 … 360.
// The current 29 lessons are worth 370 XP, so finishing everything is level 10.
const xpToReach = (level: number) => 4 * level * (level - 1);

// Medieval ranks, all respectable: everyone starts as a Novice.
const TITLES = ["Novice", "Apprentice", "Squire", "Scribe", "Knight", "Ranger", "Paladin", "Sorcerer", "Oracle", "Archmage"];

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
