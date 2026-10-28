// Weekly rhythm: a gentle alternative to daily streaks. Everything here is derived from the dates a learner
// passed lessons and challenges, so nothing needs to be stored (or repaired) besides the goal and time zone.

export const DEFAULT_WEEKLY_GOAL = 2;
export const WEEKLY_GOALS = [1, 2, 3, 5] as const;
/** One rest week is banked for every four weeks met, up to two, and is spent automatically on a missed week. */
const WEEKS_PER_REST = 4;
const MAX_RESTS = 2;

export const goalOf = (stored: number | undefined) => (stored === undefined ? DEFAULT_WEEKLY_GOAL : stored);

export function validTimezone(tz: unknown): tz is string {
  if (typeof tz !== "string" || tz.length === 0 || tz.length > 64) return false;
  try {
    new Intl.DateTimeFormat("en-GB", { timeZone: tz });
    return true;
  } catch {
    return false;
  }
}

/** Index of the Monday-to-Sunday week containing `date` in the given zone (days since the epoch, 1 Jan 1970 was a Thursday). */
export function weekIndex(date: Date, timeZone: string): number {
  const [y, m, d] = new Intl.DateTimeFormat("en-CA", { timeZone, year: "numeric", month: "2-digit", day: "2-digit" })
    .format(date)
    .split("-")
    .map(Number);
  const day = Math.floor(Date.UTC(y, m - 1, d) / 86_400_000);
  return Math.floor((day + 3) / 7);
}

export type Rhythm = {
  goal: number;
  /** Lessons and challenges first-passed this week. */
  thisWeek: number;
  /** Consecutive weeks the goal was met, counting this one once it is met. */
  current: number;
  best: number;
  /** Banked rest weeks. */
  rest: number;
};

export function computeRhythm(passedAt: Date[], storedGoal: number | undefined, timeZone: string, now = new Date()): Rhythm {
  const goal = goalOf(storedGoal);
  const tz = validTimezone(timeZone) ? timeZone : "UTC";
  const counts = new Map<number, number>();
  for (const at of passedAt) counts.set(weekIndex(at, tz), (counts.get(weekIndex(at, tz)) ?? 0) + 1);
  const thisWeekIndex = weekIndex(now, tz);
  const thisWeek = counts.get(thisWeekIndex) ?? 0;
  if (goal <= 0 || counts.size === 0) return { goal, thisWeek, current: 0, best: 0, rest: 0 };

  let streak = 0;
  let best = 0;
  let metTotal = 0;
  let rest = 0;
  for (let w = Math.min(...counts.keys()); w < thisWeekIndex; w++) {
    if ((counts.get(w) ?? 0) >= goal) {
      streak++;
      metTotal++;
      if (metTotal % WEEKS_PER_REST === 0) rest = Math.min(MAX_RESTS, rest + 1);
      best = Math.max(best, streak);
    } else if (rest > 0) {
      rest--; // a banked rest week covers it; the run neither grows nor ends
    } else {
      streak = 0;
    }
  }
  const current = streak + (thisWeek >= goal ? 1 : 0);
  return { goal, thisWeek, current, best: Math.max(best, current), rest };
}
