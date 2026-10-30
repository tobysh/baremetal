import "server-only";
import { getUserDoc } from "@/lib/auth";
import { clientIp, rateLimit } from "@/lib/rate-limit";
import { hmac } from "@/lib/crypto";
import { sameOrigin } from "@/lib/http";
import { createShellHandlers, type ShellHandlers } from "./web";

const g = globalThis as typeof globalThis & { __bmShell?: ShellHandlers };

/** One registry per process, kept across dev-server module reloads. */
export const shell = (g.__bmShell ??= createShellHandlers({
  getUser: async () => {
    const user = await getUserDoc();
    // Accounts created before email verification existed have no flag and count as verified, as in the tutor route.
    return user ? { id: user._id.toHexString(), emailVerified: user.emailVerified !== false } : null;
  },
  sameOrigin,
  rateLimit,
  hash: hmac,
  clientIp,
  socketPath: () => process.env.SHELL_BROKER_SOCKET || undefined,
}));
