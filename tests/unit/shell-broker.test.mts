import test from "node:test";
import assert from "node:assert/strict";
import { mkdtemp, rm } from "node:fs/promises";
import { connect } from "node:net";
import os from "node:os";
import path from "node:path";
import { createBroker, DEFAULT_LIMITS, type AuditEvent, type BrokerLimits } from "../../src/lib/shell/broker";
import { encodeFrame, FrameParser, FrameType } from "../../src/lib/shell/frames";
import type { Vm } from "../../src/lib/shell/vm";

/** A VM whose vsock socket never answers, so every session ends as boot_failed after bootMs. */
const deadVm = async (): Promise<Vm> => ({ socketPath: "/nonexistent/v.sock", exited: new Promise(() => {}), destroy: async () => {} });

async function withBroker(limits: Partial<BrokerLimits>, clock: () => number, fn: (open: (account: string) => Promise<string>, audit: AuditEvent[]) => Promise<void>) {
  const dir = await mkdtemp(path.join(os.tmpdir(), "bm-broker-test-"));
  const sock = path.join(dir, "b.sock");
  const audit: AuditEvent[] = [];
  const broker = createBroker({ startVm: deadVm, limits, audit: (e) => audit.push(e), clock });
  await new Promise<void>((r) => broker.server.listen(sock, r));
  /** Opens a session and resolves with the code of the error frame that ends it. */
  const open = (account: string) =>
    new Promise<string>((resolve, reject) => {
      const c = connect(sock);
      const parser = new FrameParser();
      c.on("connect", () => c.write(encodeFrame(FrameType.Open, Buffer.from(JSON.stringify({ account, cols: 80, rows: 24 })))));
      c.on("data", (chunk) => {
        for (const f of parser.push(chunk)) if (f.type === FrameType.Error) resolve((JSON.parse(f.payload.toString()) as { code: string }).code);
      });
      c.on("close", () => resolve("closed"));
      c.on("error", reject);
    });
  try {
    await fn(open, audit);
  } finally {
    await broker.shutdown();
    await rm(dir, { recursive: true, force: true });
  }
}

test("connection cap follows the session cap", () => {
  const broker = createBroker({ startVm: deadVm, limits: { maxSessions: 7 } });
  assert.equal(broker.server.maxConnections, 7 * 2 + 8);
  assert.equal(createBroker({ startVm: deadVm }).server.maxConnections, DEFAULT_LIMITS.maxSessions * 2 + 8);
});

test("an account that used its daily VM time is refused until the next UTC day", async () => {
  let now = Date.UTC(2026, 10, 10, 12);
  const clock = () => now;
  // Each session lasts bootMs (200 ms) of real time; the fake clock moves 10 minutes while it runs.
  const limits = { bootMs: 200, dailyMs: 15 * 60_000 };
  await withBroker(limits, clock, async (open, audit) => {
    const session = (account: string) => {
      const ended = open(account);
      setTimeout(() => (now += 10 * 60_000), 50);
      return ended;
    };
    const account = "acct-aaaaaaaa";
    assert.equal(await session(account), "boot_failed");
    // 5 minutes left: admitted, and cut when they run out (the clock jumps past them).
    assert.equal(await session(account), "boot_failed");
    assert.equal(await open(account), "quota");
    assert.ok(audit.some((e) => e.event === "rejected" && e.reason === "quota"));
    assert.equal(await open("acct-bbbbbbbb"), "boot_failed", "other accounts are unaffected");
    now = Date.UTC(2026, 10, 11, 0, 1);
    assert.equal(await open(account), "boot_failed", "the quota resets at midnight UTC");
  });
});

test("a session is cut at the remaining daily time, not the full absolute limit", async () => {
  let now = Date.UTC(2026, 10, 10, 12);
  // bootMs and absoluteMs are far longer than the daily allowance, so the allowance is what ends each session.
  const limits = { bootMs: 10_000, absoluteMs: 60_000, dailyMs: 1_000 };
  await withBroker(limits, () => now, async (open) => {
    const account = "acct-cccccccc";
    const first = open(account);
    setTimeout(() => (now += 960), 5); // the fake clock charges this session 960 ms
    assert.equal(await first, "quota"); // cut after the whole allowance (1 s of real time)
    // 40 ms are left: the next session is admitted but cut after those, long before bootMs.
    const started = Date.now();
    assert.equal(await open(account), "quota");
    assert.ok(Date.now() - started < 2_000);
  });
});
