import "server-only";
import { randomBytes } from "node:crypto";
import { BrokerError, openBroker as openBrokerSocket, type BrokerConnection } from "./client";

/*
 * Transport: Server-Sent Events for output, plain POSTs for input/resize, DELETE to close.
 * Next route handlers cannot upgrade to WebSocket, and a separate WS listener would need its own port, proxy
 * rule, CSRF/origin handling and a second copy of the session check. SSE is a normal same-origin HTTPS response
 * (cookie auth, CSP connect-src 'self' and the existing proxy all just work). The cost is one POST per input
 * batch; the client coalesces keystrokes so that stays cheap. The session lives in this process's memory under
 * an unguessable token bound to the account and to the sign-in session that created it, so it needs a single web
 * process (or sticky routing) and nothing is stored in the database, cookies or localStorage. Terminal content is
 * never logged. The session binding is rechecked on every request and every heartbeat, so signing out, a revoked or
 * expired session, a suspension or a deleted account ends the shell within one heartbeat even when it happened in
 * another process; killUser() ends it at once in this one.
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
  /** Shells open at once from one client address (quartered for the unknown-address bucket by ipLimit). */
  maxPerIp: 4,
  inputPerSession: { burst: 64 * 1024, perSec: 16 * 1024 },
  /** Every input POST costs at least this many bytes of the input bucket, so empty POSTs are not free. */
  minInputCost: 64,
  /** Resize requests per session (each one signals the guest's shell). */
  resizePerSession: { burst: 10, perSec: 2 },
} as const;

export type ShellUser = {
  id: string;
  emailVerified: boolean;
  /** Hash of the session token that authenticated this request (the `tokenHash` of its sessions record). */
  session: string;
};

export type ShellDeps = {
  getUser: () => Promise<ShellUser | null>;
  sameOrigin: (req: Request) => boolean;
  /** True when the call is allowed. */
  rateLimit: (key: string, limit: number, windowSeconds: number) => Promise<boolean>;
  /** Opaque keyed hash; the broker never sees the real id. */
  hash: (purpose: string, data: string) => string;
  clientIp: (headers: Headers) => string;
  /** Per-IP limit for this address (smaller for the shared unknown-address bucket). */
  ipLimit: (ip: string, limit: number) => number;
  socketPath: () => string | undefined;
  /**
   * The live token hash for a bound session: the same hash, its successor after a token rotation, or null once the
   * session was signed out, revoked or expired, or the account was suspended or deleted.
   */
  sessionCurrent: (session: string) => Promise<string | null>;
  /** Connects to the broker (tests pass a fake). */
  openBroker?: typeof openBrokerSocket;
};

type Bucket = { tokens: number; last: number };
/** Refill, then take `cost` if the bucket holds it. */
function take(bucket: Bucket, burst: number, perSec: number, cost: number): boolean {
  const now = Date.now();
  bucket.tokens = Math.min(burst, bucket.tokens + ((now - bucket.last) / 1000) * perSec);
  bucket.last = now;
  if (cost > bucket.tokens) return false;
  bucket.tokens -= cost;
  return true;
}

type Entry = {
  token: string;
  account: string;
  userId: string;
  /** Hash of the creating sign-in session's token; follows token rotation. */
  session: string;
  ip: string;
  conn: BrokerConnection;
  attached: boolean;
  attachTimer: NodeJS.Timeout;
  input: Bucket;
  resize: Bucket;
  checking?: Promise<void>;
  destroy: () => void;
};

/** A create that is still connecting to the broker; killUser() marks it so it is closed as soon as it connects. */
type Pending = { pending: true; userId: string; cancelled: boolean };

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

const STATUS: Record<string, number> = { busy: 503, account_busy: 409, quota: 429, unavailable: 503, boot_failed: 503, broker_shutdown: 503, bad_open: 400 };

export function createShellHandlers(deps: ShellDeps) {
  const openBroker = deps.openBroker ?? openBrokerSocket;
  const byToken = new Map<string, Entry>();
  const byAccount = new Map<string, Entry | Pending>();
  /** Shells (live or being created) per hashed client address. */
  const byIp = new Map<string, number>();
  const addIp = (ip: string, n: number) => {
    const next = (byIp.get(ip) ?? 0) + n;
    if (next > 0) byIp.set(ip, next);
    else byIp.delete(ip);
  };

  /** Common gate: same origin, signed in, email verified. */
  async function gate(req: Request): Promise<{ user: ShellUser } | { response: Response }> {
    if (!deps.sameOrigin(req)) return { response: err(403, "Cross-site request blocked.") };
    const user = await deps.getUser();
    if (!user) return { response: err(401, "Sign in to use the shell.") };
    if (!user.emailVerified) return { response: err(403, "Verify your email address to use the shell.") };
    return { user };
  }

  /**
   * The caller's own live shell, opened from this sign-in session. An unknown token, someone else's token and a token
   * presented by another session of the same account all look the same (404). A rotated session token is followed to
   * its successor; a session that has ended destroys the shell.
   */
  async function own(user: ShellUser, token: string): Promise<Entry | null> {
    const entry = byToken.get(token);
    if (!entry || entry.userId !== user.id) return null;
    if (entry.session === user.session) return entry;
    const current = await revalidate(entry);
    return current === user.session && byToken.get(token) === entry ? entry : null;
  }

  /** Re-reads the bound session; destroys the shell if it ended. Resolves with the live session hash, or null. */
  async function revalidate(entry: Entry): Promise<string | null> {
    // One lookup at a time per shell: a heartbeat and a request arriving together share it.
    entry.checking ??= (async () => {
      try {
        const current = await deps.sessionCurrent(entry.session);
        if (current) entry.session = current;
        else entry.destroy();
      } catch {
        // A failed lookup (database down) is not proof that the session ended; the next heartbeat tries again.
      } finally {
        entry.checking = undefined;
      }
    })();
    await entry.checking;
    return byToken.get(entry.token) === entry ? entry.session : null;
  }

  function release(entry: Entry) {
    clearTimeout(entry.attachTimer);
    if (!byToken.delete(entry.token)) return;
    addIp(entry.ip, -1);
    if (byAccount.get(entry.account) === entry) byAccount.delete(entry.account);
    entry.conn.close();
  }

  /** Ends every shell of this user in this process, including ones still being created. Returns how many. */
  function killUser(userId: string): number {
    let n = 0;
    for (const value of byAccount.values()) {
      if ("pending" in value && value.userId === userId && !value.cancelled) {
        value.cancelled = true;
        n++;
      }
    }
    for (const entry of [...byToken.values()]) {
      if (entry.userId !== userId) continue;
      entry.destroy();
      n++;
    }
    return n;
  }

  return {
    stats: () => ({ sessions: byToken.size, accounts: byAccount.size, addresses: byIp.size }),
    killUser,
    /** Rechecks every shell's session now (the heartbeat does this per shell). */
    revalidateAll: async () => void (await Promise.all([...byToken.values()].map(revalidate))),

    async create(req: Request): Promise<Response> {
      const g = await gate(req);
      if ("response" in g) return g.response;
      const { user } = g;
      const rawIp = deps.clientIp(req.headers);
      const ip = deps.hash("rate-ip", rawIp);
      const { createPerUser: u, createPerIp: i } = LIMITS;
      if (!(await deps.rateLimit(`shell:create:user:${user.id}`, u.limit, u.windowSeconds)) || !(await deps.rateLimit(`shell:create:ip:${ip}`, deps.ipLimit(rawIp, i.limit), i.windowSeconds))) {
        return err(429, "Too many shell sessions started. Try again in a few minutes.");
      }
      const size = parseSize(await readBytes(req, LIMITS.createBody));
      if (!size) return err(400, "Invalid request.");
      const socketPath = deps.socketPath();
      if (!socketPath) return err(503, "The shell service is not available.", "unavailable");

      const account = deps.hash("shell-account", user.id);
      if (byAccount.has(account)) return err(409, "You already have a shell open.", "account_busy");
      if ((byIp.get(ip) ?? 0) >= deps.ipLimit(rawIp, LIMITS.maxPerIp)) return err(429, "Too many shells are open from your network. Close one and try again.", "ip_busy");
      if (byAccount.size >= LIMITS.maxLocal) return err(503, "No shell is free right now. Try again in a minute.", "busy");
      // Reserved synchronously, so two racing requests cannot both connect.
      const pending: Pending = { pending: true, userId: user.id, cancelled: false };
      byAccount.set(account, pending);
      addIp(ip, 1);
      const unreserve = () => {
        if (byAccount.get(account) === pending) byAccount.delete(account);
        addIp(ip, -1);
      };
      let conn: BrokerConnection;
      try {
        conn = await openBroker({ socketPath, account, cols: size.cols, rows: size.rows, signal: req.signal });
      } catch (error) {
        unreserve();
        if (error instanceof BrokerError) return err(STATUS[error.code] ?? 502, error.message, error.code);
        return err(502, "The shell could not start.", "boot_failed");
      }
      if (req.signal.aborted || pending.cancelled) {
        unreserve();
        conn.close();
        return pending.cancelled ? err(401, "Sign in to use the shell.") : err(499, "Cancelled.");
      }
      const token = randomBytes(32).toString("base64url");
      const now = Date.now();
      const entry: Entry = {
        token, account, userId: user.id, session: user.session, ip, conn, attached: false,
        input: { tokens: LIMITS.inputPerSession.burst, last: now },
        resize: { tokens: LIMITS.resizePerSession.burst, last: now },
        attachTimer: setTimeout(() => release(entry), LIMITS.attachMs),
        destroy: () => release(entry),
      };
      entry.attachTimer.unref();
      byToken.set(token, entry); // takes over the address reservation; release() gives it back
      byAccount.set(account, entry);
      conn.pause(); // output is held in the socket until the stream attaches; the attach timer reaps unclaimed sessions
      return json(201, { token });
    },

    /** Output stream. Cancelled or aborted requests tear the broker connection down. */
    async stream(req: Request, token: string): Promise<Response> {
      const g = await gate(req);
      if ("response" in g) return g.response;
      const entry = await own(g.user, token);
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
                // A destroyed entry closes the broker connection, whose onClose finishes this stream.
                void revalidate(entry);
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
      const entry = await own(g.user, token);
      if (!entry) return err(404, "No such shell.");
      if (req.headers.get("content-type")?.split(";")[0].trim() !== "application/octet-stream") return err(415, "Unsupported content type.");
      const bytes = await readBytes(req, LIMITS.inputBody);
      if (!bytes) return err(413, "Input too large.");
      const { burst, perSec } = LIMITS.inputPerSession;
      if (!take(entry.input, burst, perSec, Math.max(bytes.length, LIMITS.minInputCost))) return err(429, "Input is arriving too fast.");
      if (bytes.length) entry.conn.write(bytes);
      return new Response(null, { status: 204, headers: NO_STORE });
    },

    async resize(req: Request, token: string): Promise<Response> {
      const g = await gate(req);
      if ("response" in g) return g.response;
      const entry = await own(g.user, token);
      if (!entry) return err(404, "No such shell.");
      const size = parseSize(await readBytes(req, LIMITS.resizeBody));
      if (!size) return err(400, "Invalid request.");
      const { burst, perSec } = LIMITS.resizePerSession;
      if (!take(entry.resize, burst, perSec, 1)) return err(429, "Resizing too often.");
      entry.conn.resize(size.cols, size.rows);
      return new Response(null, { status: 204, headers: NO_STORE });
    },

    async close(req: Request, token: string): Promise<Response> {
      const g = await gate(req);
      if ("response" in g) return g.response;
      (await own(g.user, token))?.destroy();
      return new Response(null, { status: 204, headers: NO_STORE });
    },
  };
}

export type ShellHandlers = ReturnType<typeof createShellHandlers>;
