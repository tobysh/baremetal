// Pure helpers for the review schedule. No server-only import, so scripts and checks can use them.
import { validTimezone } from "./rhythm";

export const DAY = 86_400_000;

/**
 * The one place a due date gets its random spread: the interval plus or minus 10%, with at least one day either
 * way for intervals of a week or more. Spreads lessons passed together so they do not all fall due on one day.
 * `rand` returns [0, 1) and is injectable for tests.
 */
export function jitterDays(intervalDays: number, rand: () => number = Math.random): number {
  const span = intervalDays >= 7 ? Math.max(1, intervalDays * 0.1) : intervalDays * 0.1;
  return intervalDays + (rand() * 2 - 1) * span;
}

/** Due date for a review of `intervalDays`, counted from `from`, with jitter applied. */
export const dueAfter = (from: Date, intervalDays: number, rand?: () => number) => new Date(from.getTime() + jitterDays(intervalDays, rand) * DAY);

const parts = (date: Date, tz: string) => {
  const f = new Intl.DateTimeFormat("en-CA", { timeZone: tz, hourCycle: "h23", year: "numeric", month: "2-digit", day: "2-digit", hour: "2-digit", minute: "2-digit", second: "2-digit" }).formatToParts(date);
  const n = (t: string) => Number(f.find((p) => p.type === t)!.value);
  return { y: n("year"), m: n("month"), d: n("day"), h: n("hour"), mi: n("minute"), s: n("second") };
};

/** Offset of `tz` from UTC at `date`, in ms. */
function offsetAt(date: Date, tz: string) {
  const p = parts(date, tz);
  return Date.UTC(p.y, p.m - 1, p.d, p.h, p.mi, p.s) - Math.floor(date.getTime() / 1000) * 1000;
}

/** The instant that is 00:00 on the local calendar day `daysAhead` after the day containing `date`, in `timeZone` (UTC when invalid). */
export function startOfLocalDay(date: Date, timeZone: string | undefined, daysAhead = 0): Date {
  const tz = validTimezone(timeZone) ? timeZone : "UTC";
  const p = parts(date, tz);
  const wall = Date.UTC(p.y, p.m - 1, p.d + daysAhead);
  // The offset can differ between the wall time and `date` across a clocks change, so settle it in two passes.
  let guess = wall - offsetAt(date, tz);
  guess = wall - offsetAt(new Date(guess), tz);
  return new Date(guess);
}
