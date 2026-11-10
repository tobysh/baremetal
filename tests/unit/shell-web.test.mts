import test from "node:test";
import assert from "node:assert/strict";
import { createShellHandlers, LIMITS, type ShellDeps, type ShellUser } from "../../src/lib/shell/web";
import { killUser } from "../../src/lib/shell/kill";
import type { BrokerConnection, BrokerHandlers } from "../../src/lib/shell/client";

type FakeConn = BrokerConnection & { closed: boolean; writes: Buffer[]; resizes: Array<[number, number]> };

function fakeConn(): FakeConn {
  let handlers: BrokerHandlers | undefined;
  const c: FakeConn = {
    closed: false,
    writes: [],
    resizes: [],
    write: (b) => void c.writes.push(Buffer.from(b)),
    resize: (cols, rows) => void c.resizes.push([cols, rows]),
    pause: () => {},
    resume: () => {},
    close: () => {
      if (c.closed) return;
      c.closed = true;
      handlers?.onClose();
    },
    setHandlers: (h) => void (handlers = h),
  };
  return c;
}

const UNKNOWN = "unknown-ip";

function setup(overrides: Partial<ShellDeps> = {}) {
  let user: ShellUser | null = { id: "u1", emailVerified: true, session: "s1" };
  /** session -> what sessionCurrent returns; missing means "still the same session". */
  const live = new Map<string, string | null>();
  const conns: FakeConn[] = [];
  const deps: ShellDeps = {
    getUser: async () => user,
    sameOrigin: () => true,
    rateLimit: async () => true,
    hash: (purpose, data) => `${purpose}:${data}`,
    clientIp: (h) => h.get("x-ip") ?? UNKNOWN,
    ipLimit: (ip, limit) => (ip === UNKNOWN ? Math.max(1, Math.floor(limit / 4)) : limit),
    socketPath: () => "/nonexistent/broker.sock",
    sessionCurrent: async (s) => (live.has(s) ? live.get(s)! : s),
    openBroker: async () => {
      const c = fakeConn();
      conns.push(c);
      return c;
    },
    ...overrides,
  };
  const shell = createShellHandlers(deps);
  return {
    shell,
    conns,
    live,
    as: (id: string, session = `s-${id}`) => void (user = { id, emailVerified: true, session }),
  };
}

const create = (shell: ReturnType<typeof createShellHandlers>, ip?: string) =>
  shell.create(new Request("http://x/api/shell", { method: "POST", body: JSON.stringify({ cols: 80, rows: 24 }), headers: ip ? { "x-ip": ip } : {} }));
const input = (shell: ReturnType<typeof createShellHandlers>, token: string, body: BodyInit = new Uint8Array(0)) =>
  shell.input(new Request(`http://x/api/shell/${token}/input`, { method: "POST", body, headers: { "content-type": "application/octet-stream" } }), token);
const resize = (shell: ReturnType<typeof createShellHandlers>, token: string) =>
  shell.resize(new Request(`http://x/api/shell/${token}/resize`, { method: "POST", body: JSON.stringify({ cols: 100, rows: 30 }) }), token);
const tokenOf = async (res: Response) => ((await res.json()) as { token: string }).token;

test("concurrent shells are capped per IP, and a closed one frees its place", async () => {
  const t = setup();
  const tokens: string[] = [];
  for (let i = 0; i < LIMITS.maxPerIp; i++) {
    t.as(`user${i}`);
    const res = await create(t.shell, "203.0.113.5");
    assert.equal(res.status, 201);
    tokens.push(await tokenOf(res));
  }
  t.as("another");
  const refused = await create(t.shell, "203.0.113.5");
  assert.equal(refused.status, 429);
  assert.equal(((await refused.json()) as { code?: string }).code, "ip_busy");
  assert.equal((await create(t.shell, "198.51.100.7")).status, 201, "another address is not affected");
  t.as("user0");
  await t.shell.close(new Request("http://x", { method: "DELETE" }), tokens[0]);
  t.as("third");
  assert.equal((await create(t.shell, "203.0.113.5")).status, 201);
});

test("the unknown-IP bucket gets the reduced ipLimit() allowance", async () => {
  const t = setup();
  const cap = Math.max(1, Math.floor(LIMITS.maxPerIp / 4));
  for (let i = 0; i < cap; i++) {
    t.as(`anon${i}`);
    assert.equal((await create(t.shell)).status, 201);
  }
  t.as("anon-extra");
  assert.equal((await create(t.shell)).status, 429);
});

test("empty input POSTs still cost tokens", async () => {
  const t = setup();
  const token = await tokenOf(await create(t.shell, "203.0.113.5"));
  const { burst, perSec } = LIMITS.inputPerSession;
  const started = Date.now();
  let accepted = 0;
  let limited = false;
  for (let i = 0; i < (4 * burst) / LIMITS.minInputCost; i++) {
    const res = await input(t.shell, token);
    if (res.status === 429) {
      limited = true;
      break;
    }
    assert.equal(res.status, 204);
    accepted++;
  }
  assert.ok(limited, `empty POSTs were never limited (${accepted} accepted)`);
  // The burst plus whatever refilled while the loop ran, at the minimum cost per request.
  const refill = ((Date.now() - started) / 1000) * perSec;
  assert.ok(accepted <= Math.ceil((burst + refill) / LIMITS.minInputCost) + 1, `${accepted} accepted`);
});

test("resize is rate-limited", async () => {
  const t = setup();
  const token = await tokenOf(await create(t.shell, "203.0.113.5"));
  let accepted = 0;
  for (let i = 0; i < LIMITS.resizePerSession.burst + 5; i++) {
    const res = await resize(t.shell, token);
    if (res.status === 429) break;
    accepted++;
  }
  assert.equal(accepted, LIMITS.resizePerSession.burst);
  assert.equal(t.conns[0].resizes.length, accepted);
});

test("a shell only answers the session that created it, following a token rotation", async () => {
  const t = setup();
  t.as("u1", "s1");
  const token = await tokenOf(await create(t.shell, "203.0.113.5"));
  // Same account, another browser session: refused, and the shell stays up.
  t.as("u1", "other");
  assert.equal((await input(t.shell, token, Buffer.from("a"))).status, 404);
  assert.equal(t.conns[0].closed, false);
  // The creating session's token was rotated to s2: accepted and rebound.
  t.live.set("s1", "s2");
  t.as("u1", "s2");
  assert.equal((await input(t.shell, token, Buffer.from("b"))).status, 204);
  // The bound session (now s2) was signed out: the shell is destroyed.
  t.live.set("s2", null);
  t.as("u1", "s3");
  assert.equal((await input(t.shell, token, Buffer.from("c"))).status, 404);
  assert.equal(t.conns[0].closed, true);
  assert.equal(t.shell.stats().sessions, 0);
});

test("the heartbeat recheck destroys a shell whose session ended", async () => {
  const t = setup();
  t.as("u1", "s1");
  const token = await tokenOf(await create(t.shell, "203.0.113.5"));
  const res = await t.shell.stream(new Request(`http://x/api/shell/${token}`), token);
  assert.equal(res.status, 200);
  await t.shell.revalidateAll();
  assert.equal(t.conns[0].closed, false, "a live session keeps its shell");
  t.live.set("s1", null);
  await t.shell.revalidateAll();
  assert.equal(t.conns[0].closed, true);
  assert.equal(t.shell.stats().sessions, 0);
  await res.body?.cancel().catch(() => {});
});

test("killUser ends that account's shells only", async () => {
  const t = setup();
  t.as("u1");
  await create(t.shell, "203.0.113.5");
  t.as("u2");
  await create(t.shell, "198.51.100.7");
  assert.equal(t.shell.killUser("u1"), 1);
  assert.equal(t.conns[0].closed, true);
  assert.equal(t.conns[1].closed, false);
  assert.equal(t.shell.stats().sessions, 1);
});

test("killUser also cancels a shell that is still being created", async () => {
  let release!: () => void;
  const gate = new Promise<void>((r) => (release = r));
  const conns: FakeConn[] = [];
  const t = setup({
    openBroker: async () => {
      await gate;
      const c = fakeConn();
      conns.push(c);
      return c;
    },
  });
  t.as("u1");
  const pending = create(t.shell, "203.0.113.5");
  await new Promise((r) => setImmediate(r));
  t.shell.killUser("u1");
  release();
  const res = await pending;
  assert.notEqual(res.status, 201);
  assert.equal(conns[0].closed, true);
  assert.equal(t.shell.stats().sessions, 0);
  assert.equal((await create(t.shell, "203.0.113.5")).status, 201, "the account and IP reservations were released");
});

test("the process-wide killUser reaches the registered handlers", async () => {
  const t = setup();
  const g = globalThis as typeof globalThis & { __bmShell?: unknown };
  const previous = g.__bmShell;
  g.__bmShell = t.shell;
  try {
    t.as("u9");
    await create(t.shell, "203.0.113.5");
    killUser("u9");
    assert.equal(t.conns[0].closed, true);
  } finally {
    g.__bmShell = previous;
  }
  delete g.__bmShell;
  assert.doesNotThrow(() => killUser("nobody"), "no shell registered is not an error");
});
