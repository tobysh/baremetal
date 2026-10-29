/** Only allow same-site relative redirects after login. */
export function safeNext(next: unknown, fallback = "/dashboard"): string {
  if (typeof next !== "string" || !next.startsWith("/") || /[\\\x00-\x20\x7f]/.test(next) || /%(?:0[0-9a-f]|1[0-9a-f]|7f|5c)/i.test(next)) return fallback;
  try {
    const url = new URL(next, "http://x");
    return url.origin === "http://x" ? url.pathname + url.search : fallback;
  } catch {
    return fallback;
  }
}

