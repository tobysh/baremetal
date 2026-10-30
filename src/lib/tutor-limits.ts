// Layered limits on model calls, so one account, one machine, or everyone
// together can't run up an unbounded bill. Answers served from the response
// cache cost nothing and don't count. All configurable in .env.local.
const env = (name: string, fallback: number) => Math.max(0, Number(process.env[name]) || fallback);
export const LIMITS = {
  userPerHour: env("TUTOR_LIMIT_USER_HOUR", 20),
  userPerDay: env("TUTOR_LIMIT_USER_DAY", 100),
  ipPerHour: env("TUTOR_LIMIT_IP_HOUR", 120), // accounts under 24 h old only: established accounts are bounded by the per-user limits and the dollar budget
  globalPerHour: env("TUTOR_LIMIT_GLOBAL_HOUR", 2000),
  requestsPerHour: 120, // all requests, including cache hits
  cacheHitsIpPerHour: env("TUTOR_LIMIT_CACHE_HIT_IP_HOUR", 300), // cache hits cost no dollars but can still be scraped
};
export const HOUR = 3600;

