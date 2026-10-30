// Checks for the shell broker. Benign commands only: nothing here tries to harm or escape the guest.
//   SHELL_KERNEL=... SHELL_ROOTFS=... FIRECRACKER=... npx tsx scripts/shell/broker-checks.mts
// Without those variables only the fake-guest checks run. Hostile-workload tests belong on a disposable VM.
import { mkdtemp, readdir, rm } from "node:fs/promises";
import { connect, createServer, type Server, type Socket } from "node:net";
import os from "node:os";
import path from "node:path";
import { createBroker, type AuditEvent, type BrokerLimits } from "../../src/lib/shell/broker";
import { encodeFrame, FrameParser, FrameType, resizePayload, type Frame } from "../../src/lib/shell/frames";
import { startUnjailedVm, type Vm } from "../../src/lib/shell/vm";

let failures = 0;
function check(name: string, ok: boolean, detail?: unknown) {
  if (!ok) failures++;
  console.log(`${ok ? "PASS" : "FAIL"}  ${name}${ok ? "" : `  ${detail === undefined ? "" : JSON.stringify(detail)}`}`);
}
const sleep = (ms: number) => new Promise((r) => setTimeout(r, ms));
const until = async (fn: () => boolean, ms: number) => { const end = Date.now() + ms; while (!fn() && Date.now() < end) await sleep(25); return fn(); };

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
}

await rm(dir, { recursive: true, force: true });
console.log(failures ? `\n${failures} check(s) FAILED` : "\nall checks passed");
process.exit(failures ? 1 : 0);
