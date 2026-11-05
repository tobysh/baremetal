import type { UserDoc } from "./models";

/** True while an administrator's lock or ban is in force. Suspended accounts cannot sign in and existing sessions stop working. */
export function isSuspended(user: Pick<UserDoc, "suspension">, now = Date.now()): boolean {
  const s = user.suspension;
  return !!s && (s.kind === "banned" || !s.until || s.until.getTime() > now);
}
