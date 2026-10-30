import "server-only";
import { randomBytes } from "node:crypto";
import { BrokerError, openBroker, type BrokerConnection } from "./client";

/*
 * Transport: Server-Sent Events for output, plain POSTs for input/resize, DELETE to close.
 * Next route handlers cannot upgrade to WebSocket, and a separate WS listener would need its own port, proxy
 * rule, CSRF/origin handling and a second copy of the session check. SSE is a normal same-origin HTTPS response
 * (cookie auth, CSP connect-src 'self' and the existing proxy all just work). The cost is one POST per input
 * batch; the client coalesces keystrokes so that stays cheap. The session lives in this process's memory under
 * an unguessable token bound to the account, so it needs a single web process (or sticky routing) and nothing
 * is stored in the database, cookies or localStorage. Terminal content is never logged.
 */

export const LIMITS = {
  createBody: 256,
  inputBody: 4096,
  resizeBody: 64,
  maxCols: 500,
  maxRows: 200,
  /** A created session must be streamed within this time or it is closed. */
  attachMs: 15_000,
  heartbeatMs: 15_000,
  /** Sessions this web process will hold at once; the broker has its own, smaller cap. */
  maxLocal: 16,
  createPerUser: { limit: 6, windowSeconds: 600 },
  createPerIp: { limit: 20, windowSeconds: 600 },
  inputPerSession: { burst: 64 * 1024, perSec: 16 * 1024 },
} as const;

export type ShellUser = { id: string; emailVerified: boolean };

export type ShellDeps = {
  getUser: () => Promise<ShellUser | null>;
  sameOrigin: (req: Request) => boolean;
  /** True when the call is allowed. */
  rateLimit: (key: string, limit: number, windowSeconds: number) => Promise<boolean>;
  /** Opaque keyed hash; the broker never sees the real id. */
  hash: (purpose: string, data: string) => string;
  clientIp: (headers: Headers) => string;
  socketPath: () => string | undefined;
};

type Entry = {
  token: string;
  account: string;
  userId: string;
  conn: BrokerConnection;
  attached: boolean;
  attachTimer: NodeJS.Timeout;
  tokens: number;
  last: number;
  destroy: () => void;
};

const NO_STORE = { "Cache-Control": "no-store" };
const json = (status: number, body: Record<string, unknown>) => Response.json(body, { status, headers: NO_STORE });
const err = (status: number, error: string, code?: string) => json(status, code ? { error, code } : { error });

/** Reads a body without buffering more than `max` bytes (works for chunked bodies with no Content-Length). */
async function readBytes(req: Request, max: number): Promise<Buffer | null> {
  if (Number(req.headers.get("content-length") ?? 0) > max) return null;
  if (!req.body) return Buffer.alloc(0);
  const reader = req.body.getReader();
  const chunks: Uint8Array[] = [];
  let size = 0;
  for (;;) {
    const { done, value } = await reader.read();
    if (done) break;
    size += value.byteLength;
    if (size > max) {
      await reader.cancel().catch(() => {});
      return null;
    }
    chunks.push(value);
  }
  return Buffer.concat(chunks);
}

const dimension = (n: unknown, max: number): n is number => typeof n === "number" && Number.isInteger(n) && n >= 1 && n <= max;
function parseSize(bytes: Buffer | null): { cols: number; rows: number } | null {
  if (!bytes) return null;
  try {
    const v = JSON.parse(bytes.toString("utf8")) as { cols?: unknown; rows?: unknown };
    return dimension(v.cols, LIMITS.maxCols) && dimension(v.rows, LIMITS.maxRows) ? { cols: v.cols, rows: v.rows } : null;
  } catch {
    return null;
  }
}

const STATUS: Record<string, number> = { busy: 503, account_busy: 409, unavailable: 503, boot_failed: 503, broker_shutdown: 503, bad_open: 400 };

export function createShellHandlers(deps: ShellDeps) {
  const byToken = new Map<string, Entry>();
  const byAccount = new Map<string, Entry | "pending">();

  /** Common gate: same origin, signed in, email verified. */
  async function gate(req: Request): Promise<{ user: ShellUser } | { response: Response }> {
    if (!deps.sameOrigin(req)) return { response: err(403, "Cross-site request blocked.") };
    const user = await deps.getUser();
    if (!user) return { response: err(401, "Sign in to use the shell.") };
    if (!user.emailVerified) return { response: err(403, "Verify your email address to use the shell.") };
    return { user };
  }

  /** The caller's own live session. An unknown token and someone else's token look the same. */
  function own(user: ShellUser, token: string): Entry | null {
    const entry = byToken.get(token);
    return entry && entry.userId === user.id ? entry : null;
  }

  function release(entry: Entry) {
    clearTimeout(entry.attachTimer);
    byToken.delete(entry.token);
    if (byAccount.get(entry.account) === entry) byAccount.delete(entry.account);
    entry.conn.close();
  }

  return {
    stats: () => ({ sessions: byToken.size, accounts: byAccount.size }),

    async create(req: Request): Promise<Response> {
      const g = await gate(req);
      if ("response" in g) return g.response;
      const { user } = g;
      const ip = deps.hash("rate-ip", deps.clientIp(req.headers));
      const { createPerUser: u, createPerIp: i } = LIMITS;
      if (!(await deps.rateLimit(`shell:create:user:${user.id}`, u.limit, u.windowSeconds)) || !(await deps.rateLimit(`shell:create:ip:${ip}`, i.limit, i.windowSeconds))) {
        return err(429, "Too many shell sessions started. Try again in a few minutes.");
      }
      const size = parseSize(await readBytes(req, LIMITS.createBody));
      if (!size) return err(400, "Invalid request.");
      const socketPath = deps.socketPath();
      if (!socketPath) return err(503, "The shell service is not available.", "unavailable");

      const account = deps.hash("shell-account", user.id);
      if (byAccount.has(account)) return err(409, "You already have a shell open.", "account_busy");
      if (byToken.size >= LIMITS.maxLocal) return err(503, "No shell is free right now. Try again in a minute.", "busy");
      byAccount.set(account, "pending"); // reserved synchronously, so two racing requests cannot both connect
      let conn: BrokerConnection;
      try {
        conn = await openBroker({ socketPath, account, cols: size.cols, rows: size.rows, signal: req.signal });
      } catch (error) {
        byAccount.delete(account);
        if (error instanceof BrokerError) return err(STATUS[error.code] ?? 502, error.message, error.code);
        return err(502, "The shell could not start.", "boot_failed");
      }
      if (req.signal.aborted) {
        byAccount.delete(account);
        conn.close();
        return err(499, "Cancelled.");
      }
      const token = randomBytes(32).toString("base64url");
      const entry: Entry = {
        token, account, userId: user.id, conn, attached: false, tokens: LIMITS.inputPerSession.burst, last: Date.now(),
        attachTimer: setTimeout(() => release(entry), LIMITS.attachMs),
        destroy: () => release(entry),
      };
      entry.attachTimer.unref();
      byToken.set(token, entry);
      byAccount.set(account, entry);
      conn.pause(); // output is held in the socket until the stream attaches; the attach timer reaps unclaimed sessions
      return json(201, { token });
    },

    /** Output stream. Cancelled or aborted requests tear the broker connection down. */
    async stream(req: Request, token: string): Promise<Response> {
      const g = await gate(req);
      if ("response" in g) return g.response;
      const entry = own(g.user, token);
      if (!entry) return err(404, "No such shell.");
      if (entry.attached) return err(409, "This shell is already attached.");
      entry.attached = true;
      clearTimeout(entry.attachTimer);
      const encoder = new TextEncoder();
      let heartbeat: NodeJS.Timeout | undefined;
      let finished = false;
      const body = new ReadableStream<Uint8Array>(
        {
          start(controller) {
            const send = (event: string | null, data: string) => {
              if (finished) return;
              try {
                controller.enqueue(encoder.encode(`${event ? `event: ${event}\n` : ""}data: ${data}\n\n`));
              } catch {
                finish();
              }
            };
            const finish = () => {
              if (finished) return;
              finished = true;
              clearInterval(heartbeat);
              entry.destroy();
              try {
                controller.close();
              } catch {}
            };
            heartbeat = setInterval(() => {
              if (!finished) {
                try {
                  controller.enqueue(encoder.encode(": keepalive\n\n"));
                } catch {
                  finish();
                }
              }
            }, LIMITS.heartbeatMs);
            heartbeat.unref();
            req.signal.addEventListener("abort", finish, { once: true });
            entry.conn.setHandlers({
              onData(chunk) {
                send(null, chunk.toString("base64"));
                // Backpressure: stop reading the broker until the browser has drained the queue.
                if ((controller.desiredSize ?? 1) <= 0) entry.conn.pause();
              },
              onExit: (status) => {
                send("exit", JSON.stringify({ status }));
                finish();
              },
              onError: (e) => {
                send("shell-error", JSON.stringify({ code: e.code, message: e.message }));
                finish();
              },
              onClose: finish,
            });
            send("ready", "{}");
          },
          pull() {
            entry.conn.resume();
          },
          cancel() {
            finished = true;
            clearInterval(heartbeat);
            entry.destroy();
          },
        },
        { highWaterMark: 256 * 1024, size: (chunk) => chunk.byteLength },
      );
      return new Response(body, {
        headers: { "Content-Type": "text/event-stream; charset=utf-8", "Cache-Control": "no-store, no-transform", "X-Accel-Buffering": "no", Connection: "keep-alive" },
      });
    },

    async input(req: Request, token: string): Promise<Response> {
      const g = await gate(req);
      if ("response" in g) return g.response;
      const entry = own(g.user, token);
      if (!entry) return err(404, "No such shell.");
      if (req.headers.get("content-type")?.split(";")[0].trim() !== "application/octet-stream") return err(415, "Unsupported content type.");
      const bytes = await readBytes(req, LIMITS.inputBody);
      if (!bytes) return err(413, "Input too large.");
      const now = Date.now();
      const { burst, perSec } = LIMITS.inputPerSession;
      entry.tokens = Math.min(burst, entry.tokens + ((now - entry.last) / 1000) * perSec);
      entry.last = now;
      if (bytes.length > entry.tokens) return err(429, "Input is arriving too fast.");
      entry.tokens -= bytes.length;
      if (bytes.length) entry.conn.write(bytes);
      return new Response(null, { status: 204, headers: NO_STORE });
    },

    async resize(req: Request, token: string): Promise<Response> {
      const g = await gate(req);
      if ("response" in g) return g.response;
      const entry = own(g.user, token);
      if (!entry) return err(404, "No such shell.");
      const size = parseSize(await readBytes(req, LIMITS.resizeBody));
      if (!size) return err(400, "Invalid request.");
      entry.conn.resize(size.cols, size.rows);
      return new Response(null, { status: 204, headers: NO_STORE });
    },

    async close(req: Request, token: string): Promise<Response> {
      const g = await gate(req);
      if ("response" in g) return g.response;
      own(g.user, token)?.destroy();
      return new Response(null, { status: 204, headers: NO_STORE });
    },
  };
}

export type ShellHandlers = ReturnType<typeof createShellHandlers>;
