import "server-only";
import { currentSessionHash, getUserDoc, liveSessionHash } from "@/lib/auth";
import { clientIp, ipLimit, rateLimit } from "@/lib/rate-limit";
import { hmac } from "@/lib/crypto";
import { sameOrigin } from "@/lib/http";
import { createShellHandlers, type ShellHandlers } from "./web";

const g = globalThis as typeof globalThis & { __bmShell?: ShellHandlers };

/** One registry per process, kept across dev-server module reloads. `killUser` in ./kill.ts reaches it through `g`. */
export const shell = (g.__bmShell ??= createShellHandlers({
  getUser: async () => {
    const user = await getUserDoc();
    const session = user && (await currentSessionHash());
    // Accounts created before email verification existed have no flag and count as verified, as in the tutor route.
    return user && session ? { id: user._id.toHexString(), emailVerified: user.emailVerified !== false, session } : null;
  },
  sameOrigin,
  rateLimit,
  hash: hmac,
  clientIp,
  ipLimit,
  socketPath: () => process.env.SHELL_BROKER_SOCKET || undefined,
  sessionCurrent: liveSessionHash,
}));
