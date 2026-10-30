// Checks for the shell broker. Benign commands only: nothing here tries to harm or escape the guest.
//   SHELL_KERNEL=... SHELL_ROOTFS=... FIRECRACKER=... npx tsx scripts/shell/broker-checks.mts
// Without those variables only the fake-guest checks run. Hostile-workload tests belong on a disposable VM.
import { spawn } from "node:child_process";
import { mkdtemp, readdir, rm, stat } from "node:fs/promises";
import { connect, createServer, type Server, type Socket } from "node:net";
import os from "node:os";
import path from "node:path";
import { createBroker, type AuditEvent, type BrokerLimits } from "../../src/lib/shell/broker";
import { encodeFrame, FrameParser, FrameType, resizePayload, type Frame } from "../../src/lib/shell/frames";
import { createLauncher, type LauncherLimits } from "../../src/lib/shell/launcher";
import { launcherVmStarter } from "../../src/lib/shell/launcher-client";
import { startUnjailedVm, type Vm } from "../../src/lib/shell/vm";

let failures = 0;
function check(name: string, ok: boolean, detail?: unknown) {
  if (!ok) failures++;
  console.log(`${ok ? "PASS" : "FAIL"}  ${name}${ok ? "" : `  ${detail === undefined ? "" : JSON.stringify(detail)}`}`);
}
const sleep = (ms: number) => new Promise((r) => setTimeout(r, ms));
const until = async (fn: () => boolean, ms: number) => { const end = Date.now() + ms; while (!fn() && Date.now() < end) await sleep(25); return fn(); };
const untilAsync = async (fn: () => Promise<boolean>, ms: number) => { const end = Date.now() + ms; while (!(await fn()) && Date.now() < end) await sleep(50); return fn(); };

const { SHELL_KERNEL, SHELL_ROOTFS, FIRECRACKER } = process.env;
const real = !!(SHELL_KERNEL && SHELL_ROOTFS && FIRECRACKER);
const dir = await mkdtemp(path.join(os.tmpdir(), "bm-broker-checks-"));

/** A web-side client: sends frames, collects everything the broker sends back. */
class Client {
  frames: Frame[] = [];
  closed = false;
  private parser = new FrameParser();
  readonly socket: Socket;
  constructor(socketPath: string) {
    this.socket = connect(socketPath);
    this.socket.on("data", (c) => this.frames.push(...this.parser.push(c)));
    this.socket.on("close", () => (this.closed = true));
    this.socket.on("error", () => {});
  }
  send(type: number, payload?: Uint8Array) { this.socket.write(encodeFrame(type, payload)); }
  open(account: string, cols = 80, rows = 24) { this.send(FrameType.Open, Buffer.from(JSON.stringify({ account, cols, rows }))); }
  get output() { return Buffer.concat(this.frames.filter((f) => f.type === FrameType.Data).map((f) => f.payload)).toString("utf8"); }
  get error(): string | undefined { const f = this.frames.find((x) => x.type === FrameType.Error); return f ? JSON.parse(f.payload.toString()).code : undefined; }
  get opened() { return this.frames.some((f) => f.type === FrameType.Opened); }
  get exitStatus(): number | undefined { const f = this.frames.find((x) => x.type === FrameType.Exit); return f?.payload.readInt32BE(0); }
}

async function withBroker<T>(startVm: () => Promise<Vm>, limits: Partial<BrokerLimits>, fn: (b: ReturnType<typeof createBroker>, connectClient: () => Client, audit: AuditEvent[]) => Promise<T>) {
  const socketPath = path.join(dir, `b-${Math.random().toString(36).slice(2)}.sock`);
  const audit: AuditEvent[] = [];
  const broker = createBroker({ startVm, limits, audit: (e) => audit.push(e) });
  await new Promise<void>((r) => broker.server.listen(socketPath, r));
  try {
    return await fn(broker, () => new Client(socketPath), audit);
  } finally {
    await broker.shutdown();
  }
}

const hostFirecrackers = async () => {
  let n = 0;
  for (const pid of await readdir("/proc")) {
    if (!/^\d+$/.test(pid)) continue;
    try { if ((await import("node:fs/promises").then((fs) => fs.readFile(`/proc/${pid}/cmdline`, "utf8"))).includes("bm-vm-")) n++; } catch {}
  }
  return n;
};
const vmDirs = async () => (await readdir(os.tmpdir())).filter((n) => n.startsWith("bm-vm-")).length;
const noLeftovers = async (label: string, broker: ReturnType<typeof createBroker>) => {
  const gone = await until(() => broker.stats().reserved === 0, 5000);
  await sleep(300);
  check(`${label}: capacity released`, gone, broker.stats());
  check(`${label}: no firecracker process left`, (await hostFirecrackers()) === 0);
  check(`${label}: no VM directory left`, (await vmDirs()) === 0);
};

// ---------------------------------------------------------------- fake guests (always run)

/** A scripted guest: the broker connects to it exactly as it would to Firecracker's vsock proxy. */
async function fakeVm(script: (guest: Socket) => void): Promise<{ startVm: () => Promise<Vm>; destroyed: () => number }> {
  let destroys = 0;
  const startVm = async (): Promise<Vm> => {
    const sock = path.join(dir, `g-${Math.random().toString(36).slice(2)}.sock`);
    const server: Server = createServer((guest) => {
      guest.once("data", () => { guest.write("OK 1\n"); script(guest); }); // handshake line, then the script
      guest.on("error", () => {});
    });
    await new Promise<void>((r) => server.listen(sock, r));
    return { socketPath: sock, exited: new Promise(() => {}), destroy: async () => { destroys++; server.close(); } };
  };
  return { startVm, destroyed: () => destroys };
}

{
  const g = await fakeVm((guest) => guest.write(encodeFrame(FrameType.Ready)));
  await withBroker(g.startVm, {}, async (broker, mk) => {
    const c = mk(); c.open("account-aaaaaaaa");
    check("fake guest: reaches OPENED", await until(() => c.opened, 3000), c.frames);
    c.socket.destroy();
    check("fake guest: client disconnect destroys the VM", await until(() => g.destroyed() === 1, 3000), g.destroyed());
    check("fake guest: capacity released after disconnect", await until(() => broker.stats().reserved === 0, 3000));
  });
}
{
  const g = await fakeVm((guest) => { guest.write(Buffer.from([FrameType.Ready, 0, 0, 0, 0])); guest.write(Buffer.from([FrameType.Data, 0x7f, 0xff, 0xff, 0xff])); });
  await withBroker(g.startVm, {}, async (_b, mk) => {
    const c = mk(); c.open("account-bbbbbbbb");
    check("hostile guest: oversize frame length ends the session", await until(() => c.error === "guest_protocol", 3000), c.error);
    check("hostile guest: its VM is destroyed", await until(() => g.destroyed() === 1, 3000));
  });
}
{
  const g = await fakeVm((guest) => guest.write(encodeFrame(FrameType.Data, Buffer.from("before ready"))));
  await withBroker(g.startVm, {}, async (_b, mk) => {
    const c = mk(); c.open("account-cccccccc");
    check("hostile guest: data before READY is a protocol violation", await until(() => c.error === "guest_protocol", 3000), c.error);
    check("hostile guest: nothing reached the client", !c.output.includes("before ready"));
  });
}
{
  const g = await fakeVm(() => {}); // never becomes ready
  await withBroker(g.startVm, { bootMs: 800 }, async (_b, mk) => {
    const c = mk(); c.open("account-dddddddd");
    check("silent guest: boot timeout", await until(() => c.error === "boot_failed", 3000), c.error);
    check("silent guest: VM destroyed", await until(() => g.destroyed() === 1, 3000));
  });
}
{
  const g = await fakeVm((guest) => { guest.write(encodeFrame(FrameType.Ready)); const chunk = Buffer.alloc(16 * 1024, 65); const t = setInterval(() => guest.write(encodeFrame(FrameType.Data, chunk)), 1); guest.on("close", () => clearInterval(t)); });
  await withBroker(g.startVm, { maxOutputBytes: 200_000 }, async (_b, mk) => {
    const c = mk(); c.open("account-eeeeeeee");
    check("output flood: cut off at the cap", await until(() => c.error === "output_limit", 5000), c.error);
    check("output flood: client received no more than the cap", c.output.length <= 200_000, c.output.length);
  });
}
{
  const started: number[] = [];
  const startVm = async (): Promise<Vm> => { started.push(Date.now()); await sleep(600); return { socketPath: "/nonexistent", exited: new Promise(() => {}), destroy: async () => { started.push(-1); } }; };
  await withBroker(startVm, { bootMs: 1500 }, async (broker, mk) => {
    const c = mk(); c.open("account-ffffffff");
    await sleep(150); c.socket.destroy(); // leaves while the VM is still starting
    check("disconnect during VM start: the late VM is still destroyed", await until(() => started.includes(-1), 4000), started);
    check("disconnect during VM start: capacity released", await until(() => broker.stats().reserved === 0, 4000));
  });
}
{
  const startVm = async (): Promise<Vm> => { throw new Error("no kvm"); };
  await withBroker(startVm, {}, async (broker, mk) => {
    const c = mk(); c.open("account-gggggggg");
    check("VM start failure: reported as boot_failed", await until(() => c.error === "boot_failed", 3000), c.error);
    check("VM start failure: capacity released", await until(() => broker.stats().reserved === 0, 3000));
  });
}
{
  const g = await fakeVm((guest) => guest.write(encodeFrame(FrameType.Ready)));
  await withBroker(g.startVm, { maxSessions: 1 }, async (broker, mk, audit) => {
    const a = mk(); a.open("account-hhhhhhhh");
    await until(() => a.opened, 3000);
    const sameAccount = mk(); sameAccount.open("account-hhhhhhhh");
    check("second shell for one account is refused", await until(() => sameAccount.error === "account_busy", 3000), sameAccount.error);
    const other = mk(); other.open("account-iiiiiiii");
    check("global cap refuses another account", await until(() => other.error === "busy", 3000), other.error);
    check("the first session survived both refusals", a.opened && !a.closed && broker.stats().reserved === 1, broker.stats());
    check("refusals are audited without content", audit.filter((e) => e.event === "rejected").length === 2, audit);
  });
}
{
  const g = await fakeVm((guest) => guest.write(encodeFrame(FrameType.Ready)));
  await withBroker(g.startVm, {}, async (_b, mk) => {
    const bad = [
      ["garbage JSON", () => { const c = mk(); c.send(FrameType.Open, Buffer.from("{nope")); return c; }],
      ["bad account id", () => { const c = mk(); c.open("short"); return c; }],
      ["wrong first frame", () => { const c = mk(); c.send(FrameType.Data, Buffer.from("hi")); return c; }],
      ["oversize length header", () => { const c = mk(); c.socket.write(Buffer.from([FrameType.Open, 0x7f, 0xff, 0xff, 0xff])); return c; }],
    ] as const;
    for (const [name, make] of bad) { const c = make(); check(`bad OPEN (${name}) is closed`, await until(() => c.closed, 3000), c.error); }
    const silent = mk();
    check("a client that never sends OPEN is dropped", await until(() => silent.closed, 8000));
  });
}
{
  const g = await fakeVm((guest) => guest.write(encodeFrame(FrameType.Ready)));
  await withBroker(g.startVm, { inputBurst: 100, inputPerSec: 10 }, async (_b, mk) => {
    const c = mk(); c.open("account-jjjjjjjj"); await until(() => c.opened, 3000);
    c.send(FrameType.Data, Buffer.alloc(2000, 97));
    check("input flood: session closed", await until(() => c.error === "input_rate", 3000), c.error);
    const d = mk(); d.open("account-kkkkkkkk"); await until(() => d.opened, 3000);
    d.send(FrameType.Resize, resizePayload(0, 10));
    check("invalid resize is a protocol violation", await until(() => d.error === "protocol", 3000), d.error);
  });
}

{
  // A guest writing ~16 MB/s against a 256 KiB/s bucket: slowed by backpressure, not killed, until the flood cutoff.
  const g = await fakeVm((guest) => { guest.write(encodeFrame(FrameType.Ready)); const chunk = Buffer.alloc(16 * 1024, 66); const t = setInterval(() => guest.write(encodeFrame(FrameType.Data, chunk)), 1); guest.on("close", () => clearInterval(t)); });
  const lim = { outputBurst: 64 * 1024, outputPerSec: 256 * 1024, outputFloodMs: 3000, maxOutputBytes: 1e9 };
  await withBroker(g.startVm, lim, async (_b, mk) => {
    const c = mk(); c.open("account-rrrrrrrr"); await until(() => c.opened, 3000);
    const t0 = Date.now();
    await sleep(1500);
    const elapsed = (Date.now() - t0) / 1000;
    const allowed = lim.outputBurst + lim.outputPerSec * elapsed + 128 * 1024; // plus one socket read of overshoot
    check("output rate: fast guest is throttled to the bucket", c.output.length <= allowed && c.output.length > lim.outputPerSec * 0.5, { got: c.output.length, allowed });
    check("output rate: throttling alone does not end the session", !c.closed && c.error === undefined, c.error);
    check("output rate: sustained flood ends the session", await until(() => c.error === "output_flood", 6000), c.error);
    const at = Date.now() - t0;
    check("output rate: cutoff comes after the flood window, not before", at >= lim.outputFloodMs, at);
  });
}
{
  // Under the rate: 100 KiB/s against 256 KiB/s, with a short flood window, survives and loses nothing.
  let sent = 0;
  const g = await fakeVm((guest) => { guest.write(encodeFrame(FrameType.Ready)); const chunk = Buffer.alloc(2048, 67); const t = setInterval(() => { sent += chunk.length; guest.write(encodeFrame(FrameType.Data, chunk)); }, 20); guest.on("close", () => clearInterval(t)); });
  await withBroker(g.startVm, { outputBurst: 64 * 1024, outputPerSec: 256 * 1024, outputFloodMs: 1000 }, async (_b, mk) => {
    const c = mk(); c.open("account-ssssssss"); await until(() => c.opened, 3000);
    await sleep(2500);
    check("output rate: a guest under the rate is never cut off", !c.closed && c.error === undefined, c.error);
    check("output rate: and all of its output arrives", await until(() => c.output.length >= sent - 4096, 1000), { got: c.output.length, sent });
  });
}
{
  // A burst within the bucket arrives at once even with a tiny sustained rate.
  const g = await fakeVm((guest) => { guest.write(encodeFrame(FrameType.Ready)); for (let i = 0; i < 3; i++) guest.write(encodeFrame(FrameType.Data, Buffer.alloc(16 * 1024, 68))); });
  await withBroker(g.startVm, { outputBurst: 64 * 1024, outputPerSec: 1024 }, async (_b, mk) => {
    const c = mk(); c.open("account-tttttttt"); await until(() => c.opened, 3000);
    check("output rate: a burst within the bucket is not delayed", await until(() => c.output.length === 48 * 1024, 300), c.output.length);
  });
}

// ---------------------------------------------------------------- launcher (privileged half), fake VMs

/** Fake VMs for the launcher: count starts and destroys, and let a check end a VM from the "guest" side. */
function fakeLaunchVms(opts: { startMs?: number; destroyMs?: number; failStart?: boolean; failDestroy?: boolean } = {}) {
  const log = { started: [] as number[], destroyed: 0, destroyedAt: [] as number[], exit: [] as (() => void)[] };
  const startVm = async (slot: number): Promise<Vm> => {
    await sleep(opts.startMs ?? 0);
    if (opts.failStart) throw new Error("no kvm");
    log.started.push(slot);
    let exit!: () => void;
    const exited = new Promise<void>((r) => (exit = r));
    log.exit.push(exit);
    let done: Promise<void> | undefined;
    return { socketPath: `/run/fake/vm-${slot}.sock`, exited, destroy: () => (done ??= (async () => {
      await sleep(opts.destroyMs ?? 0);
      if (opts.failDestroy) throw new Error("still alive");
      log.destroyed++; log.destroyedAt.push(Date.now()); exit();
    })()) };
  };
  return { startVm, log };
}

/** A raw protocol client, for sending things the real client never would. */
class Raw {
  lines: string[] = [];
  closed = false;
  closedAt = 0;
  readonly socket: Socket;
  constructor(socketPath: string) {
    this.socket = connect(socketPath);
    let buf = "";
    this.socket.on("data", (c) => { buf += c; let nl; while ((nl = buf.indexOf("\n")) >= 0) { this.lines.push(buf.slice(0, nl)); buf = buf.slice(nl + 1); } });
    this.socket.on("close", () => { this.closed = true; this.closedAt = Date.now(); });
    this.socket.on("error", () => {});
  }
  send(text: string) { this.socket.write(text); }
  has(fragment: string) { return this.lines.some((l) => l.includes(fragment)); }
}

async function withLauncher<T>(startVm: (slot: number) => Promise<Vm>, limits: Partial<LauncherLimits>, fn: (l: ReturnType<typeof createLauncher>, socketPath: string) => Promise<T>) {
  const socketPath = path.join(dir, `l-${Math.random().toString(36).slice(2)}.sock`);
  const launcher = createLauncher({ startVm, limits, log: () => {} });
  await new Promise<void>((r) => launcher.server.listen(socketPath, r));
  try {
    return await fn(launcher, socketPath);
  } finally {
    await launcher.shutdown();
  }
}

{
  const f = fakeLaunchVms({ destroyMs: 200 });
  await withLauncher(f.startVm, {}, async (l, sock) => {
    const a = new Raw(sock); a.send(`{"op":"create"}\n`);
    check("launcher: create returns the VM's socket", await until(() => a.has(`"ok":true`) && a.has("/run/fake/vm-0.sock"), 2000), a.lines);
    a.socket.destroy(); // the broker vanishes
    check("launcher: a dropped connection destroys the VM", await until(() => f.log.destroyed === 1, 2000), f.log);
    check("launcher: and frees its slot", await until(() => l.stats().active === 0, 2000), l.stats());
    const b = new Raw(sock); b.send(`{"op":"create"}\n`); await until(() => b.has(`"ok":true`), 2000);
    const asked = Date.now(); b.send(`{"op":"destroy"}\n`);
    await until(() => b.closed, 3000);
    check("launcher: destroy answers exited only after the VM is gone", b.has(`"event":"exited"`) && f.log.destroyedAt[1] <= b.closedAt && b.closedAt - asked >= 200, { lines: b.lines, waited: b.closedAt - asked });
  });
}
{
  const f = fakeLaunchVms();
  await withLauncher(f.startVm, { maxVms: 2 }, async (l, sock) => {
    const a = new Raw(sock); a.send(`{"op":"create"}\n`);
    const b = new Raw(sock); b.send(`{"op":"create"}\n`);
    await until(() => a.has(`"ok":true`) && b.has(`"ok":true`), 2000);
    const c = new Raw(sock); c.send(`{"op":"create"}\n`);
    check("launcher: refuses beyond maxVms", await until(() => c.has(`"code":"busy"`) && c.closed, 2000), c.lines);
    check("launcher: slots are distinct", new Set(f.log.started).size === 2, f.log.started);
    a.socket.end();
    await until(() => l.stats().active === 1, 2000);
    const d = new Raw(sock); d.send(`{"op":"create"}\n`);
    check("launcher: a freed slot is reused", await until(() => d.has(`"ok":true`), 2000), d.lines);
  });
}
{
  const f = fakeLaunchVms();
  await withLauncher(f.startVm, { requestMs: 500 }, async (l, sock) => {
    const bad: [string, string][] = [
      ["garbage", "nope\n"],
      ["extra parameter", `{"op":"create","kernel":"/etc/shadow"}\n`],
      ["unknown op", `{"op":"reap"}\n`],
      ["destroy before create", `{"op":"destroy"}\n`],
      ["oversize line", `${"x".repeat(300)}\n`],
      ["no newline flood", "y".repeat(4096)],
    ];
    for (const [name, text] of bad) {
      const r = new Raw(sock); r.send(text);
      check(`launcher: bad request (${name}) is closed without a VM`, await until(() => r.closed, 2000) && !r.has(`"ok":true`), r.lines);
    }
    const silent = new Raw(sock);
    check("launcher: a connection that never asks is dropped", await until(() => silent.closed, 2000));
    const twice = new Raw(sock); twice.send(`{"op":"create"}\n`); await until(() => twice.has(`"ok":true`), 2000);
    twice.send(`{"op":"create"}\n`);
    check("launcher: a second create ends the lease and destroys the VM", await until(() => twice.closed && f.log.destroyed === 1, 2000), f.log);
    check("launcher: no VM started for bad requests", f.log.started.length === 1 && l.stats().active === 0, { started: f.log.started, stats: l.stats() });
  });
}
{
  const f = fakeLaunchVms();
  await withLauncher(f.startVm, { maxLifetimeMs: 600 }, async (l, sock) => {
    const r = new Raw(sock); r.send(`{"op":"create"}\n`);
    check("launcher: lifetime cap destroys a VM the broker still holds", await until(() => r.has(`"event":"exited"`) && r.closed && f.log.destroyed === 1, 3000), r.lines);
    const s2 = new Raw(sock); s2.send(`{"op":"create"}\n`); await until(() => s2.has(`"ok":true`), 2000);
    f.log.exit[1]();
    check("launcher: a VM that exits on its own is reported and cleaned up", await until(() => s2.closed && s2.has(`"event":"exited"`) && l.stats().active === 0, 2000), s2.lines);
  });
}
{
  const f = fakeLaunchVms({ failStart: true });
  await withLauncher(f.startVm, {}, async (l, sock) => {
    const r = new Raw(sock); r.send(`{"op":"create"}\n`);
    check("launcher: start failure is reported and frees the slot", await until(() => r.has(`"code":"start_failed"`) && r.closed && l.stats().active === 0, 2000), r.lines);
  });
}
{
  const f = fakeLaunchVms({ failDestroy: true });
  await withLauncher(f.startVm, {}, async (l, sock) => {
    const r = new Raw(sock); r.send(`{"op":"create"}\n`); await until(() => r.has(`"ok":true`), 2000);
    r.socket.destroy();
    await sleep(200);
    check("launcher: a VM that may have survived keeps its slot (and uid) out of use", l.stats().active === 1, l.stats());
  });
}
{
  const f = fakeLaunchVms({ startMs: 400 });
  await withLauncher(f.startVm, {}, async (l, sock) => {
    const r = new Raw(sock); r.send(`{"op":"create"}\n`);
    await sleep(100); r.socket.destroy(); // gone while the VM is still starting
    check("launcher: disconnect during start still destroys the late VM", await until(() => f.log.destroyed === 1 && l.stats().active === 0, 2000), f.log);
  });
}
{
  const f = fakeLaunchVms();
  const socketPath = path.join(dir, "l-shutdown.sock");
  const launcher = createLauncher({ startVm: f.startVm, log: () => {} });
  await new Promise<void>((r) => launcher.server.listen(socketPath, r));
  const start = launcherVmStarter(socketPath);
  const [a, b] = await Promise.all([start(), start()]);
  check("launcher client: returns VM handles", a.socketPath !== b.socketPath, [a.socketPath, b.socketPath]);
  await a.destroy();
  check("launcher client: destroy resolves after the launcher destroyed the VM", f.log.destroyed === 1, f.log);
  await launcher.shutdown();
  check("launcher shutdown: destroys every leased VM", f.log.destroyed === 2, f.log);
  check("launcher client: sees the VM exit when the launcher goes away", await Promise.race([b.exited.then(() => true), sleep(2000).then(() => false)]));
  const refused = createLauncher({ startVm: f.startVm, limits: { maxVms: 1 }, log: () => {} });
  const p2 = path.join(dir, "l-refuse.sock");
  await new Promise<void>((r) => refused.server.listen(p2, r));
  const held = await launcherVmStarter(p2)();
  const err = await launcherVmStarter(p2)().then(() => "resolved", (e: Error) => e.message);
  check("launcher client: a refusal rejects with the launcher's code", err.includes("busy"), err);
  await held.destroy(); await refused.shutdown();
}
{
  // Broker and launcher together, as deployed: the broker's VMs are launcher leases.
  const g = await fakeVm((guest) => guest.write(encodeFrame(FrameType.Ready)));
  await withLauncher(() => g.startVm(), {}, async (l, sock) => {
    await withBroker(launcherVmStarter(sock), {}, async (broker, mk) => {
      const c = mk(); c.open("account-uuuuuuuu");
      check("broker via launcher: reaches OPENED", await until(() => c.opened, 3000), c.frames);
      c.socket.destroy();
      check("broker via launcher: client disconnect destroys the VM in the launcher", await until(() => g.destroyed() === 1 && l.stats().active === 0, 3000), l.stats());
      check("broker via launcher: broker capacity released", await until(() => broker.stats().reserved === 0, 3000));
      const d = mk(); d.open("account-vvvvvvvv"); await until(() => d.opened, 3000);
      await l.shutdown(); // the launcher stops: the broker must notice and end the session
      check("broker via launcher: launcher going away ends the session", await until(() => d.error === "vm_exited" || d.error === "guest_gone", 3000), d.error);
      check("broker via launcher: and releases capacity", await until(() => broker.stats().reserved === 0, 3000), broker.stats());
    });
  });
}
{
  // The jailed launcher refuses to start without root. (Nothing here runs as root.)
  if (process.getuid?.() !== 0) {
    const child = spawn(process.execPath, ["--import", "tsx", path.resolve("scripts/shell-launcher.mts")], { env: { ...process.env, SHELL_LAUNCHER_SOCKET: path.join(dir, "never.sock"), SHELL_LAUNCHER_MODE: "" }, stdio: ["ignore", "ignore", "pipe"] });
    let errText = ""; child.stderr.on("data", (c) => (errText += c));
    const code = await new Promise<number | null>((r) => child.on("close", r));
    check("launcher service: jailed mode refuses to run without root", code !== 0 && errText.includes("must run as root"), { code, errText: errText.slice(0, 300) });
  }
}

// ---------------------------------------------------------------- real microVMs

if (!real) {
  console.log("SKIP  real-VM checks (set SHELL_KERNEL, SHELL_ROOTFS, FIRECRACKER)");
} else {
  const startVm = () => startUnjailedVm({ firecracker: FIRECRACKER!, kernel: SHELL_KERNEL!, rootfs: SHELL_ROOTFS! });
  await withBroker(startVm, { idleMs: 60_000 }, async (broker, mk) => {
    const c = mk(); const t0 = Date.now(); c.open("account-llllllll", 100, 30);
    check("real VM: shell is ready", await until(() => c.opened, 20_000), c.frames.map((f) => f.type));
    console.log(`      (ready after ${Date.now() - t0} ms)`);
    c.send(FrameType.Data, Buffer.from("id; grep -E 'Cap(Eff|Bnd)|NoNewPrivs' /proc/self/status; stty size; touch /x; echo write=$?; mount -t tmpfs t /mnt; echo mount=$?; ls /sys/class/net; echo end$((20+1))\n"));
    await until(() => c.output.includes("end21"), 5000); // computed, so the echoed command line cannot match
    const out = c.output;
    check("real VM: unprivileged learner", out.includes("uid=1000(learner)"), out);
    check("real VM: no capabilities, no_new_privs", /CapEff:\s+0{16}/.test(out) && /CapBnd:\s+0{16}/.test(out) && /NoNewPrivs:\s+1/.test(out), out);
    check("real VM: initial size applied", out.includes("30 100"), out);
    check("real VM: root fs read-only, mount refused", /write=1/.test(out) && /mount=32/.test(out), out);
    check("real VM: loopback is the only network device", /^lo\s*$/m.test(out.split("mount=32")[1] ?? ""), out);
    c.send(FrameType.Resize, resizePayload(120, 40));
    c.send(FrameType.Data, Buffer.from("stty size\n"));
    check("real VM: resize reaches the PTY", await until(() => c.output.includes("40 120"), 5000), c.output.slice(-200));
    c.send(FrameType.Data, Buffer.from("exit\n"));
    check("real VM: exit status reported", await until(() => c.exitStatus === 0, 10_000), c.exitStatus);
    check("real VM: connection closed after exit", await until(() => c.closed, 5000));
    await noLeftovers("real VM after exit", broker);
  });
  await withBroker(startVm, { idleMs: 60_000 }, async (broker, mk) => {
    const c = mk(); c.open("account-mmmmmmmm"); await until(() => c.opened, 20_000);
    c.socket.destroy();
    await noLeftovers("real VM after client disconnect", broker);
  });
  await withBroker(startVm, { absoluteMs: 6000 }, async (broker, mk) => {
    const c = mk(); c.open("account-nnnnnnnn"); await until(() => c.opened, 20_000);
    check("real VM: absolute time limit", await until(() => c.error === "time_limit", 15_000), c.error);
    await noLeftovers("real VM after time limit", broker);
  });
  await withBroker(startVm, { idleMs: 3000 }, async (broker, mk) => {
    const c = mk(); c.open("account-oooooooo"); await until(() => c.opened, 20_000);
    check("real VM: idle timeout", await until(() => c.error === "idle_timeout", 15_000), c.error);
    await noLeftovers("real VM after idle timeout", broker);
  });
  await withBroker(startVm, { maxOutputBytes: 100_000, idleMs: 60_000 }, async (broker, mk) => {
    const c = mk(); c.open("account-pppppppp"); await until(() => c.opened, 20_000);
    c.send(FrameType.Data, Buffer.from("while :; do echo xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx; done\n"));
    check("real VM: runaway output hits the cap", await until(() => c.error === "output_limit", 20_000), c.error);
    await noLeftovers("real VM after output flood", broker);
  });

  // The launcher as its own process (unjailed driver, current user), with the broker as a lease holder.
  const launcherSock = path.join(dir, "launcher.sock");
  const launcherProc = spawn(process.execPath, ["--import", "tsx", path.resolve("scripts/shell-launcher.mts")], {
    env: { ...process.env, SHELL_LAUNCHER_MODE: "unjailed", SHELL_LAUNCHER_SOCKET: launcherSock, SHELL_MAX_VMS: "2" },
    stdio: ["ignore", "pipe", "inherit"],
  });
  let launcherOut = ""; launcherProc.stdout.on("data", (c) => (launcherOut += c));
  const launcherExit = new Promise<number | null>((r) => launcherProc.on("close", r));
  check("launcher service: listening", await until(() => launcherOut.includes(`"listening"`), 15_000), launcherOut);
  check("launcher service: socket is private (0600) without a broker group", ((await stat(launcherSock)).mode & 0o777) === 0o600, ((await stat(launcherSock)).mode & 0o777).toString(8));
  await withBroker(launcherVmStarter(launcherSock), { idleMs: 60_000 }, async (broker, mk) => {
    const c = mk(); c.open("account-wwwwwwww");
    check("real VM via launcher: shell is ready", await until(() => c.opened, 20_000), c.frames.map((f) => f.type));
    c.send(FrameType.Data, Buffer.from("echo via$((1+1))launcher\n"));
    check("real VM via launcher: shell works", await until(() => c.output.includes("via2launcher"), 5000), c.output.slice(-200));
    c.socket.destroy();
    await noLeftovers("real VM via launcher after client disconnect", broker);
  });
  {
    // A broker that dies holding a lease: the launcher destroys the VM.
    const r = new Raw(launcherSock); r.send(`{"op":"create"}\n`);
    check("launcher service: real VM created", await until(() => r.has(`"ok":true`), 15_000), r.lines);
    check("launcher service: its firecracker is running", (await hostFirecrackers()) === 1);
    r.socket.destroy();
    check("launcher service: dropped lease kills firecracker and removes its files", await untilAsync(async () => (await hostFirecrackers()) === 0 && (await vmDirs()) === 0, 5000));
  }
  await withBroker(launcherVmStarter(launcherSock), { idleMs: 60_000 }, async (broker, mk) => {
    const c = mk(); c.open("account-xxxxxxxx"); await until(() => c.opened, 20_000);
    process.kill(launcherProc.pid!, "SIGTERM"); // by PID
    check("launcher service: SIGTERM ends live sessions", await until(() => c.error === "vm_exited" || c.error === "guest_gone", 10_000), c.error);
    check("launcher service: exits cleanly", (await launcherExit) === 0);
    await noLeftovers("real VM after launcher shutdown", broker);
  });
}

await rm(dir, { recursive: true, force: true });
console.log(failures ? `\n${failures} check(s) FAILED` : "\nall checks passed");
process.exit(failures ? 1 : 0);
