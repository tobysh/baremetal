/** True when `origin` has the same scheme, host and port as `siteUrl`. */
export function sameSiteOrigin(origin: string, siteUrl: string): boolean {
  try {
    return new URL(origin).origin === new URL(siteUrl).origin;
  } catch {
    return false;
  }
}
