import "server-only";

/**
 * Ends every interactive shell this web process holds for the user. Called when their sessions end (sign-out,
 * revocation, suspension, deletion). It does not load the shell module: if no shell was ever opened in this process
 * there is nothing to end. Shells in other processes notice within one heartbeat (web.ts rechecks the session).
 */
export function killUser(userId: string): void {
  const g = globalThis as typeof globalThis & { __bmShell?: { killUser: (userId: string) => number } };
  g.__bmShell?.killUser(userId);
}
