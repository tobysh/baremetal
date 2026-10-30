// Hostile-workload verification driver for the microVM shell. WRITTEN, NOT YET RUN: execute only on a DISPOSABLE
// host, as root, through run.sh (which enforces the guards). See README.md in this directory.
//
// For each test it opens a session through the broker (in-process broker with the real jailed driver by default,
// or a running service with --socket), feeds a hostile guest payload, then asserts the HOST-side outcome:
// session end reason and timing, cgroup/host memory, pids and CPU against stated bounds, and that nothing is left
// behind (firecracker/jailer processes, chroots, cgroups, taps, network namespaces, mounts) and that the broker can
// still start a fresh session. Every check prints PASS/FAIL/INFO/SKIP; exit status is non-zero on any FAIL.
//
//   tsx driver.mts [--list] [--dry-run] [--only a,b] [--skip a,b] [--socket /path] [--strict]
//
// --list and --dry-run touch nothing (no VM, no root needed). Everything else refuses to run unless
// BM_DISPOSABLE_HOST=yes-this-machine-is-disposable and uid 0.
import { execFile, spawn, type ChildProcess } from "node:child_process";
import { createServer as createTcpServer, connect, type Server as NetServer, type Socket } from "node:net";
import { createSocket as createUdpSocket } from "node:dgram";
import { mkdtemp, readdir, readFile, readlink, rm, stat, writeFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { createBroker, type AuditEvent, type BrokerLimits } from "../../../src/lib/shell/broker";
import { encodeFrame, FrameParser, FrameType, resizePayload, type Frame } from "../../../src/lib/shell/frames";
import { startJailedVm } from "../../../src/lib/shell/vm";

// ------------------------------------------------------------------ arguments and configuration

const argv = process.argv.slice(2);
const flag = (name: string) => argv.includes(name);
const flagValue = (name: string) => { const i = argv.indexOf(name); return i >= 0 ? argv[i + 1] : undefined; };
const csv = (v?: string) => (v ? v.split(",").map((s) => s.trim()).filter(Boolean) : []);
const only = csv(flagValue("--only"));
const skip = csv(flagValue("--skip"));
const dryRun = flag("--dry-run");
const listOnly = flag("--list");
const strict = flag("--strict");
const socketMode = flagValue("--socket");

const here = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(here, "../../..");
const envNum = (name: string, fallback: number) => { const n = Number(process.env[name]); return process.env[name] && Number.isFinite(n) ? n : fallback; };

const cfg = {
  firecracker: process.env.FIRECRACKER ?? "",
  jailer: process.env.JAILER ?? "",
  kernel: process.env.SHELL_KERNEL ?? "",
  rootfs: process.env.SHELL_ROOTFS ?? "",
  chrootBase: process.env.SHELL_CHROOT_BASE ?? "/srv/baremetal-jailer",
  cgroupRoot: process.env.SHELL_CGROUP_ROOT ?? "/sys/fs/cgroup",
  uid: envNum("SHELL_VM_UID", 300001),
  // Must equal the startJailedVm defaults in src/lib/shell/vm.ts; inspect-jailer.sh asserts the cgroup really has them.
  cpuMax: process.env.BM_CPU_MAX ?? "50000 100000",
  memMaxBytes: envNum("BM_MEM_MAX", 400 * 1024 * 1024),
  pidsMax: envNum("BM_PIDS_MAX", 64),
};
/** The stated bounds. Every one is overridable so a failure can be re-judged, but the defaults are the pass criteria. */
const bounds = {
  hostMemDropMiB: envNum("BM_MAX_HOST_MEM_DROP_MIB", 600), // MemAvailable may fall by at most this per VM-test (400 MiB cgroup + page-cache slack)
  hostProcGrowth: envNum("BM_MAX_HOST_PROC_GROWTH", 100), // host-wide process count growth while a single VM runs
  brokerRssGrowthMiB: envNum("BM_MAX_BROKER_RSS_GROWTH_MIB", 160),
  cpuSlack: envNum("BM_CPU_SLACK", 1.35), // VM CPU time <= quota * slack over any window >= 5 s
  endGraceS: envNum("BM_END_GRACE_S", 20), // a session must be gone this long after its absolute limit
  teardownS: envNum("BM_TEARDOWN_S", 15), // VM, chroot and cgroup must disappear this long after the session ends
  bootS: envNum("BM_BOOT_S", 30),
  fdGrowth: envNum("BM_MAX_FD_GROWTH", 12),
};
const GUARD = "yes-this-machine-is-disposable";

// ------------------------------------------------------------------ reporting

let failures = 0, skips = 0;
const results: { test: string; name: string; verdict: string; detail?: string }[] = [];
function record(test: string, name: string, verdict: "PASS" | "FAIL" | "INFO" | "SKIP", detail?: unknown) {
  if (verdict === "FAIL") failures++;
  if (verdict === "SKIP") { skips++; if (strict) failures++; }
  const text = detail === undefined || detail === "" ? "" : typeof detail === "string" ? detail : JSON.stringify(detail);
  results.push({ test, name, verdict, detail: text });
  console.log(`${verdict}  ${test}: ${name}${text && verdict !== "PASS" ? `  -- ${text.slice(0, 600)}` : ""}`);
}
const sleep = (ms: number) => new Promise<void>((r) => setTimeout(r, ms));
const until = async (fn: () => boolean | Promise<boolean>, ms: number, step = 50) => {
  const end = Date.now() + ms;
  while (Date.now() < end) { if (await fn()) return true; await sleep(step); }
  return !!(await fn());
};
const MiB = 1024 * 1024;
const mib = (n: number) => `${(n / MiB).toFixed(0)} MiB`;
const rand = () => Math.random().toString(36).slice(2, 10);
const account = (tag: string) => `h-${tag.replace(/[^A-Za-z0-9]/g, "")}-${rand()}${rand()}`;

// ------------------------------------------------------------------ the web-side client (pattern from broker-checks.mts)

class Client {
  frames: Frame[] = [];
  closed = false;
  dataBytes = 0;
  private parser = new FrameParser();
  readonly socket: Socket;
  private chunks: Buffer[] = [];
  constructor(socketPath: string) {
    this.socket = connect(socketPath);
    this.socket.on("data", (c) => {
      try {
        for (const f of this.parser.push(c)) {
          this.frames.push(f);
          if (f.type === FrameType.Data) { this.dataBytes += f.payload.length; this.chunks.push(f.payload); if (this.chunks.length > 4000) this.chunks.splice(0, 2000); }
        }
      } catch { this.socket.destroy(); }
    });
    this.socket.on("close", () => (this.closed = true));
    this.socket.on("error", () => {});
  }
  send(type: number, payload?: Uint8Array) { this.socket.write(encodeFrame(type, payload)); }
  open(acct: string, cols = 100, rows = 30) { this.send(FrameType.Open, Buffer.from(JSON.stringify({ account: acct, cols, rows }))); }
  /** Keystrokes in frames of at most `chunk` bytes, optionally paced. The broker's input bucket is 64 KiB burst, 16 KiB/s. */
  async type(text: string | Buffer, chunk = 1000, pauseMs = 25) {
    const buf = typeof text === "string" ? Buffer.from(text) : text;
    for (let i = 0; i < buf.length; i += chunk) { if (this.closed) return; this.send(FrameType.Data, buf.subarray(i, i + chunk)); if (pauseMs) await sleep(pauseMs); }
  }
  /** Recent terminal output (the tail only; flood tests keep the buffer bounded). */
  get output() { return Buffer.concat(this.chunks).toString("utf8"); }
  get error(): string | undefined { const f = this.frames.find((x) => x.type === FrameType.Error); return f ? (JSON.parse(f.payload.toString()) as { code: string }).code : undefined; }
  get opened() { return this.frames.some((f) => f.type === FrameType.Opened); }
  get exitStatus(): number | undefined { const f = this.frames.find((x) => x.type === FrameType.Exit); return f?.payload.readInt32BE(0); }
  /** Why the session ended, as far as the client can tell. */
  get endReason(): string | undefined { return this.error ?? (this.exitStatus !== undefined ? "shell_exit" : undefined); }
}

// ------------------------------------------------------------------ host inspection (all read-only)

const fcBase = path.basename(cfg.firecracker || "firecracker");
const cgDir = path.join(cfg.cgroupRoot, fcBase);
const readText = (p: string) => readFile(p, "utf8").catch(() => "");
const readNum = async (p: string) => { const t = (await readText(p)).trim(); return t === "" || t === "max" ? undefined : Number(t); };
const vmIds = async () => (await readdir(cgDir).catch(() => [] as string[])).filter((n) => n.startsWith("bm-"));
const numericPids = async () => (await readdir("/proc")).filter((n) => /^\d+$/.test(n));
const kv = (text: string) => Object.fromEntries(text.split("\n").map((l) => l.split(" ")).filter((p) => p.length >= 2).map((p) => [p[0], Number(p[1])]));
const memAvailable = async () => { const m = /MemAvailable:\s+(\d+)/.exec(await readText("/proc/meminfo")); return m ? Number(m[1]) * 1024 : 0; };

type Snapshot = { procs: string[]; chroots: string[]; cgroups: string[]; links: string[]; netnsFiles: string[]; nsInodes: string[]; mounts: string[]; kvmMode: string };
async function snapshot(): Promise<Snapshot> {
  const procs: string[] = [];
  const nsSet = new Set<string>();
  for (const pid of await numericPids()) {
    const comm = (await readText(`/proc/${pid}/comm`)).trim();
    if (comm === fcBase.slice(0, 15) || comm === path.basename(cfg.jailer || "jailer").slice(0, 15)) procs.push(`${pid}:${comm}`);
    for (const t of ["net", "pid", "mnt", "user", "ipc", "uts", "cgroup"]) { const l = await readlink(`/proc/${pid}/ns/${t}`).catch(() => ""); if (l) nsSet.add(l); }
  }
  const mounts = (await readText("/proc/self/mountinfo")).split("\n").filter((l) => l.includes(cfg.chrootBase) || l.includes("/bm-"));
  const netnsFiles = [...(await readdir("/run/netns").catch(() => [] as string[])), ...(await readdir("/var/run/netns").catch(() => [] as string[]))];
  const kvm = await stat("/dev/kvm").then((s) => (s.mode & 0o7777).toString(8)).catch(() => "absent");
  return {
    procs,
    chroots: await readdir(path.join(cfg.chrootBase, fcBase)).catch(() => [] as string[]),
    cgroups: await vmIds(),
    links: await readdir("/sys/class/net").catch(() => [] as string[]),
    netnsFiles, nsInodes: [...nsSet], mounts, kvmMode: kvm,
  };
}
const added = (before: string[], now: string[]) => now.filter((x) => !before.includes(x));
const procKey = (p: string) => p.split(":")[0];

/** Compare against a baseline, waiting for slow teardown. Reports one check per category. */
async function assertClean(test: string, label: string, base: Snapshot) {
  let now = await snapshot();
  const dirty = (s: Snapshot) => added(base.procs.map(procKey), s.procs.map(procKey)).length + added(base.chroots, s.chroots).length + added(base.cgroups, s.cgroups).length + added(base.links, s.links).length + added(base.netnsFiles, s.netnsFiles).length + added(base.nsInodes, s.nsInodes).length + added(base.mounts, s.mounts).length;
  const end = Date.now() + bounds.teardownS * 1000;
  while (dirty(now) > 0 && Date.now() < end) { await sleep(500); now = await snapshot(); }
  const cat = (name: string, extra: string[]) => record(test, `${label}: no leftover ${name}`, extra.length === 0 ? "PASS" : "FAIL", extra.length ? `${extra.length} left ${extra.slice(0, 6).join(", ")} (waited ${bounds.teardownS}s)` : undefined);
  cat("firecracker/jailer processes", added(base.procs.map(procKey), now.procs.map(procKey)).map((k) => now.procs.find((p) => procKey(p) === k) ?? k));
  cat("chroot directories", added(base.chroots, now.chroots));
  cat("cgroups", added(base.cgroups, now.cgroups));
  cat("network devices (taps)", added(base.links, now.links));
  cat("named network namespaces", added(base.netnsFiles, now.netnsFiles));
  cat("namespaces still referenced by a process", added(base.nsInodes, now.nsInodes));
  cat("mounts under the chroot base", added(base.mounts, now.mounts));
  record(test, `${label}: host /dev/kvm mode unchanged (${base.kvmMode})`, now.kvmMode === base.kvmMode ? "PASS" : "FAIL", `was ${base.kvmMode}, now ${now.kvmMode}`);
}

/** Samples every VM cgroup and the host every 200 ms while a test runs, and judges the peaks against the bounds. */
class Sampler {
  private timer?: NodeJS.Timeout;
  private busy = false;
  private baseAvail = 0; private baseProcs = 0; private baseRss = 0; private baseFds = 0;
  private minAvail = Infinity; private maxProcs = 0; private maxRss = 0; private maxFds = 0; maxVms = 0;
  private perVm = new Map<string, { memPeak: number; memMax?: number; pidsPeak: number; pidsMax?: number; first?: { t: number; usec: number }; last?: { t: number; usec: number }; throttled: number; oom: number }>();
  async start() {
    this.baseAvail = await memAvailable(); this.baseProcs = (await numericPids()).length; this.baseRss = process.memoryUsage().rss;
    this.baseFds = (await readdir("/proc/self/fd").catch(() => [])).length;
    this.timer = setInterval(() => void this.tick(), 200);
    this.timer.unref();
  }
  private async tick() {
    if (this.busy) return;
    this.busy = true;
    try {
      const ids = await vmIds();
      this.maxVms = Math.max(this.maxVms, ids.length);
      for (const id of ids) {
        const d = path.join(cgDir, id);
        const s = this.perVm.get(id) ?? { memPeak: 0, pidsPeak: 0, throttled: 0, oom: 0 };
        s.memMax = (await readNum(`${d}/memory.max`)) ?? s.memMax;
        s.pidsMax = (await readNum(`${d}/pids.max`)) ?? s.pidsMax;
        s.memPeak = Math.max(s.memPeak, (await readNum(`${d}/memory.current`)) ?? 0, (await readNum(`${d}/memory.peak`)) ?? 0);
        s.pidsPeak = Math.max(s.pidsPeak, (await readNum(`${d}/pids.current`)) ?? 0, (await readNum(`${d}/pids.peak`)) ?? 0);
        const cpu = kv(await readText(`${d}/cpu.stat`));
        if (cpu.usage_usec !== undefined) { const p = { t: Date.now(), usec: cpu.usage_usec }; s.first ??= p; s.last = p; s.throttled = Math.max(s.throttled, cpu.nr_throttled ?? 0); }
        s.oom = Math.max(s.oom, kv(await readText(`${d}/memory.events`)).oom_kill ?? 0);
        this.perVm.set(id, s);
      }
      this.minAvail = Math.min(this.minAvail, await memAvailable());
      this.maxProcs = Math.max(this.maxProcs, (await numericPids()).length);
      this.maxRss = Math.max(this.maxRss, process.memoryUsage().rss);
      this.maxFds = Math.max(this.maxFds, (await readdir("/proc/self/fd").catch(() => [])).length);
    } finally { this.busy = false; }
  }
  async stop(test: string, opts: { cpuBound?: boolean; vms?: number } = {}) {
    clearInterval(this.timer);
    await this.tick();
    const quota = (() => { const [q, p] = cfg.cpuMax.split(" ").map(Number); return q && p ? q / p : 0.5; })();
    for (const [id, s] of this.perVm) {
      if (s.memMax !== undefined) record(test, `${id}: cgroup memory peak ${mib(s.memPeak)} <= memory.max ${mib(s.memMax)}`, s.memPeak <= s.memMax ? "PASS" : "FAIL", `peak ${s.memPeak} > max ${s.memMax}`);
      record(test, `${id}: memory.max is the configured ${mib(cfg.memMaxBytes)}`, s.memMax === cfg.memMaxBytes ? "PASS" : "FAIL", `memory.max=${s.memMax}`);
      if (s.pidsMax !== undefined) record(test, `${id}: cgroup pids peak ${s.pidsPeak} <= pids.max ${s.pidsMax}`, s.pidsPeak <= s.pidsMax ? "PASS" : "FAIL", `peak ${s.pidsPeak} > max ${s.pidsMax}`);
      if (s.first && s.last && s.last.t - s.first.t >= 5000) {
        const cores = (s.last.usec - s.first.usec) / 1000 / (s.last.t - s.first.t);
        const ok = cores <= quota * bounds.cpuSlack;
        record(test, `${id}: VM CPU ${cores.toFixed(2)} cores over ${((s.last.t - s.first.t) / 1000).toFixed(0)} s <= quota ${quota} x ${bounds.cpuSlack}`, ok ? "PASS" : "FAIL", `used ${cores.toFixed(2)} cores; cpu.max=${cfg.cpuMax}`);
        if (opts.cpuBound) record(test, `${id}: CPU was actually throttled (nr_throttled=${s.throttled}); proves the limit engaged`, s.throttled > 0 ? "PASS" : "INFO", "never throttled: the spinner may not have run long enough");
      }
      if (s.oom) record(test, `${id}: cgroup OOM kills on the host (memory.events oom_kill)`, "INFO", `${s.oom}: the cgroup limit engaged and the kernel killed inside it`);
    }
    const nvm = Math.max(1, opts.vms ?? this.perVm.size);
    if (this.minAvail !== Infinity) {
      const drop = this.baseAvail - this.minAvail;
      record(test, `host MemAvailable dropped ${mib(Math.max(0, drop))} <= ${bounds.hostMemDropMiB * nvm} MiB`, drop <= bounds.hostMemDropMiB * nvm * MiB ? "PASS" : "FAIL", `dropped ${mib(drop)} across ${nvm} VM(s)`);
    }
    const growth = this.maxProcs - this.baseProcs;
    record(test, `host process count grew by at most ${growth} <= ${bounds.hostProcGrowth * nvm}`, growth <= bounds.hostProcGrowth * nvm ? "PASS" : "FAIL", `grew ${growth}: guest processes must not be host processes`);
    if (!socketMode) {
      const rss = this.maxRss - this.baseRss;
      record(test, `broker/driver RSS grew ${mib(Math.max(0, rss))} <= ${bounds.brokerRssGrowthMiB} MiB`, rss <= bounds.brokerRssGrowthMiB * MiB ? "PASS" : "FAIL", `grew ${mib(rss)}`);
      const fds = this.maxFds - this.baseFds;
      record(test, `broker/driver open fds grew by ${fds} <= ${bounds.fdGrowth + 4 * nvm}`, fds <= bounds.fdGrowth + 4 * nvm ? "PASS" : "FAIL", `fd growth ${fds}`);
    }
  }
}

// ------------------------------------------------------------------ broker environments

type Env = { connect: () => Client; reserved: () => Promise<number>; audit: AuditEvent[]; close: () => Promise<void>; limits: Partial<BrokerLimits> };
const workDir = dryRun || listOnly ? "" : await mkdtemp(path.join(os.tmpdir(), "bm-hostile-"));

async function makeEnv(limits: Partial<BrokerLimits> = {}): Promise<Env> {
  if (socketMode) {
    return { connect: () => new Client(socketMode), reserved: async () => (await vmIds()).length, audit: [], close: async () => {}, limits: {} };
  }
  const audit: AuditEvent[] = [];
  const broker = createBroker({
    startVm: () => startJailedVm({ firecracker: cfg.firecracker, jailer: cfg.jailer, kernel: cfg.kernel, rootfs: cfg.rootfs, chrootBase: cfg.chrootBase, uid: cfg.uid, cgroupRoot: cfg.cgroupRoot }),
    limits: { idleMs: 120_000, absoluteMs: 120_000, ...limits },
    audit: (e) => audit.push(e),
  });
  const sock = path.join(workDir, `b-${rand()}.sock`);
  await new Promise<void>((r) => broker.server.listen(sock, r));
  return { connect: () => new Client(sock), reserved: async () => broker.stats().reserved, audit, close: () => broker.shutdown(), limits };
}

/** Run `fn` against a fresh broker with its own sampler, baseline and leftover assertions, then prove the broker still works. */
async function scoped(test: string, limits: Partial<BrokerLimits>, fn: (env: Env, sampler: Sampler) => Promise<{ cpuBound?: boolean; vms?: number } | void>) {
  const base = await snapshot();
  const env = await makeEnv(limits);
  const sampler = new Sampler();
  await sampler.start();
  let opts: { cpuBound?: boolean; vms?: number } = {};
  try {
    opts = (await fn(env, sampler)) ?? {};
  } catch (error) {
    record(test, "test body threw", "FAIL", String((error as Error)?.stack ?? error));
  }
  const drained = await until(async () => (await env.reserved()) === 0, bounds.teardownS * 1000, 100);
  record(test, `broker released all capacity within ${bounds.teardownS}s of the session ending`, drained ? "PASS" : "FAIL", `reserved=${await env.reserved()}`);
  await sampler.stop(test, opts);
  await healthy(test, env);
  await env.close().catch(() => {});
  await assertClean(test, "after test", base);
}

/** The broker that just handled the hostile session must still admit and run a fresh one. */
async function healthy(test: string, env: Env) {
  const c = env.connect();
  c.open(account("health"));
  const ok = await until(() => c.opened, bounds.bootS * 1000);
  if (!ok) return void record(test, "broker healthy: a fresh session starts afterwards", "FAIL", `error=${c.error} frames=${c.frames.map((f) => f.type)}`);
  const n = 1000 + Math.floor(Math.random() * 8000);
  await c.type(`echo health$((${n}+1))\n`);
  const up = await until(() => c.output.includes(`health${n + 1}`), 10_000);
  record(test, "broker healthy: a fresh session starts afterwards and runs a command", up ? "PASS" : "FAIL", c.output.slice(-200));
  await c.type("exit\n");
  await until(() => c.closed, 10_000);
  c.socket.destroy();
}

// ------------------------------------------------------------------ guest payload delivery

const payloadDir = path.join(here, "payloads");
const lib = await readFile(path.join(payloadDir, "_lib.sh"), "utf8");
const netScript = await readFile(path.join(here, "net", "egress.sh"), "utf8");
const parseChecks = (text: string) => text.split(/\r?\n/).filter((l) => l.startsWith("BM_CHECK\t")).map((l) => { const [, name, verdict, ...rest] = l.split("\t"); return { name, verdict, detail: rest.join("\t") }; });

/** Type a script into the learner's shell through a quoted heredoc, then run it. Echo is disabled so it is not reflected back. */
async function deliver(c: Client, body: string, header = "") {
  const marker = "BM_EOF_9c1f";
  await c.type("set +o emacs; stty -echo; PS1=; PS2=\n", 200, 0);
  await sleep(300);
  await c.type(`cat > /tmp/bm.sh <<'${marker}'\n`, 200, 0);
  await c.type(`${lib}\n${header}\n${body}\n`, 500, 30);
  await c.type(`${marker}\nbash /tmp/bm.sh\n`, 200, 0);
}

type PayloadTest = {
  id: string; title: string; file: string; header?: string;
  limits?: Partial<BrokerLimits>;
  /** The guest may never finish or may take the shell down: judge by the host, not by BM_DONE. */
  destructive?: boolean;
  doneMs: number; holdMs?: number; cpuBound?: boolean;
  /** Net tests need the lab network; skipped otherwise. */
  needsLab?: boolean;
};

const OK_ENDS = ["time_limit", "vm_exited", "guest_gone", "shell_exit", "guest_protocol", "output_limit", "client_gone"];

async function runPayload(t: PayloadTest) {
  const body = await readFile(path.join(payloadDir, t.file), "utf8");
  await scoped(t.id, { ...(t.destructive ? { absoluteMs: 60_000 } : {}), ...t.limits }, async (env) => {
    const c = env.connect();
    const started = Date.now();
    c.open(account(t.id), 100, 30);
    if (!(await until(() => c.opened, bounds.bootS * 1000))) { record(t.id, "session opened", "FAIL", `error=${c.error}`); return; }
    await deliver(c, body, t.header);
    const doneSeen = await until(() => /BM_DONE\t42/.test(c.output) || c.closed, t.doneMs);
    const checks = parseChecks(c.output);
    if (!t.destructive || checks.length) {
      for (const k of checks) {
        if (k.verdict === "FAIL") record(t.id, `guest: ${k.name}`, "FAIL", k.detail);
        else if (k.verdict === "PASS") record(t.id, `guest: ${k.name}`, "PASS");
        else record(t.id, `guest: ${k.name}`, k.verdict === "SKIP" ? "SKIP" : "INFO", k.detail);
      }
    }
    if (!t.destructive) record(t.id, "payload ran to completion (BM_DONE seen)", /BM_DONE\t42/.test(c.output) ? "PASS" : "FAIL", `session closed=${c.closed} reason=${c.endReason} tail=${JSON.stringify(c.output.slice(-300))}`);
    else if (!doneSeen) record(t.id, "payload started", "INFO", "no BM_DONE (acceptable for a destructive payload)");
    if (t.holdMs && !c.closed) await until(() => c.closed, t.holdMs); // keep the load running so the sampler sees it
    if (t.destructive) {
      const limit = (env.limits.absoluteMs ?? 60_000) + bounds.endGraceS * 1000;
      const ended = socketMode ? c.closed || (await until(() => c.closed, 30_000)) : await until(() => c.closed, Math.max(0, limit - (Date.now() - started)));
      const reason = c.endReason ?? (c.closed ? "closed_without_reason" : "still_open");
      record(t.id, `session ended by the broker or VM within ${Math.round((Date.now() - started) / 1000)}s (limit ${Math.round(limit / 1000)}s), reason ${reason}`, ended ? "PASS" : socketMode ? "INFO" : "FAIL", ended ? undefined : `still open after the absolute limit + ${bounds.endGraceS}s grace: the broker did not enforce its cap on a hostile guest`);
      if (ended) record(t.id, `end reason '${reason}' is an expected containment outcome`, OK_ENDS.includes(reason) || reason === "closed_without_reason" ? "PASS" : "FAIL", `unexpected end reason ${reason}`);
    } else if (c.endReason && c.endReason !== "shell_exit") {
      record(t.id, `session survived a non-destructive payload (ended: ${c.endReason})`, "FAIL", c.endReason);
    }
    if (!c.closed) { c.socket.destroy(); }
    return { cpuBound: t.cpuBound };
  });
}

const PAYLOADS: PayloadTest[] = [
  { id: "forkbomb", title: "Shell fork bomb (RLIMIT_NPROC 256, host pids unaffected)", file: "forkbomb.sh", destructive: true, doneMs: 40_000, holdMs: 15_000 },
  { id: "memhog", title: "Memory exhaustion by string doubling x3 (guest OOM, host cgroup 400 MiB)", file: "memhog.sh", destructive: true, doneMs: 60_000, holdMs: 15_000 },
  { id: "tmpfill", title: "Fill /tmp, the home tmpfs and the inode table; read-only root", file: "tmpfill.sh", doneMs: 90_000 },
  { id: "cpuspin", title: "Four busy loops (cpu.max throttling)", file: "cpuspin.sh", destructive: true, doneMs: 40_000, holdMs: 20_000, cpuBound: true },
  { id: "ptyabuse", title: "Long lines, binary garbage, escape sequences, tty ioctls on the PTY", file: "ptyabuse.sh", doneMs: 60_000 },
  { id: "devices", title: "/dev/kvm, mem, kmem, port, kmsg, block devices, kcore", file: "devices.sh", doneMs: 30_000 },
  { id: "mounts-ns", title: "mount/umount, remount rw, missing tools, setuid files", file: "mounts-ns.sh", doneMs: 30_000 },
  { id: "procsys", title: "/proc/sys and /sys writability, host information leaks", file: "procsys.sh", doneMs: 60_000 },
  { id: "ptrace", title: "Cross-process access to PID 1 and the root agent", file: "ptrace.sh", doneMs: 30_000 },
  { id: "privesc", title: "su, setuid, capabilities, no_new_privs", file: "privesc.sh", doneMs: 30_000 },
  { id: "signal-init", title: "Signalling PID 1 and the agent", file: "signal-init.sh", doneMs: 30_000 },
  { id: "probe", title: "Syscall probe (unshare, chroot, setns, ptrace, mknod, vsock-to-host); needs the probe rootfs", file: "probe.sh", doneMs: 60_000 },
];

// ------------------------------------------------------------------ custom tests

/** Prove the shell still answers after something hostile happened to the session. */
async function responsive(c: Client, ms = 8000) {
  const n = 2000 + Math.floor(Math.random() * 7000);
  await c.type(`\necho alive$((${n}+1))\n`, 200, 0);
  return until(() => c.output.includes(`alive${n + 1}`) || c.closed, ms).then(() => c.output.includes(`alive${n + 1}`));
}

async function outputFlood() {
  const body = await readFile(path.join(payloadDir, "outflood.sh"), "utf8");
  const cap = 4 * MiB;
  await scoped("outflood", { maxOutputBytes: cap, idleMs: 120_000, absoluteMs: 90_000 }, async (env) => {
    const c = env.connect(); const t0 = Date.now();
    c.open(account("outflood")); await until(() => c.opened, bounds.bootS * 1000);
    await deliver(c, body);
    const cut = await until(() => c.error === "output_limit" || c.closed, 60_000);
    record("outflood", `session cut at the output cap in ${Math.round((Date.now() - t0) / 1000)}s`, c.error === "output_limit" ? "PASS" : "FAIL", `cut=${cut} reason=${c.endReason}`);
    record("outflood", `client received ${c.dataBytes} bytes <= cap ${cap}`, c.dataBytes <= cap ? "PASS" : "FAIL", `${c.dataBytes} bytes delivered past the cap`);
    await until(() => c.closed, 5000);
    record("outflood", "client connection closed by the broker", c.closed ? "PASS" : "FAIL");
  });
  // Slow reader: the client stops reading. The broker must apply backpressure (bounded memory) and the absolute cap must still end the session.
  await scoped("outflood-slow-reader", { maxOutputBytes: 256 * MiB, idleMs: 120_000, absoluteMs: 45_000 }, async (env) => {
    const c = env.connect(); const t0 = Date.now();
    c.open(account("slowread")); await until(() => c.opened, bounds.bootS * 1000);
    await deliver(c, body);
    await sleep(1500);
    c.socket.pause(); // stop reading: kernel buffers fill, the broker's backpressure must hold the guest
    const ended = await until(() => c.closed || env.audit.some((a) => a.event === "session_end"), 45_000 + bounds.endGraceS * 1000);
    const end = env.audit.find((a) => a.event === "session_end");
    record("outflood-slow-reader", `session ended despite a client that never reads (${Math.round((Date.now() - t0) / 1000)}s)`, ended ? "PASS" : "FAIL", "still alive past absolute limit + grace");
    record("outflood-slow-reader", `end reason '${end && "reason" in end ? end.reason : "?"}' is time_limit/output_limit/client_gone`, end && "reason" in end && ["time_limit", "output_limit", "client_gone", "vm_exited"].includes(end.reason) ? "PASS" : "FAIL", end);
    c.socket.resume(); c.socket.destroy();
  });
}

async function inputAbuse() {
  await scoped("input-abuse", { idleMs: 120_000 }, async (env) => {
    // 1. One enormous line without a newline (within the 64 KiB burst), then a newline: shell must stay usable.
    let c = env.connect(); c.open(account("longline")); await until(() => c.opened, bounds.bootS * 1000);
    await c.type(Buffer.alloc(60_000, 0x41), 8000, 10);
    record("input-abuse", "60 KB line without newline: session still open", !c.closed ? "PASS" : "FAIL", c.endReason);
    record("input-abuse", "60 KB line: shell responsive afterwards", (await responsive(c)) ? "PASS" : "FAIL", c.output.slice(-200));
    c.socket.destroy();
    // 2. Binary garbage (fixed seed) including NUL, ETX, EOT, ESC, DEL. A Ctrl-D may legitimately end the shell.
    c = env.connect(); c.open(account("garbage")); await until(() => c.opened, bounds.bootS * 1000);
    let x = 0x2545f491; const junk = Buffer.alloc(32_000);
    for (let i = 0; i < junk.length; i++) { x ^= x << 13; x ^= x >>> 17; x ^= x << 5; junk[i] = x & 0xff; }
    await c.type(junk, 4000, 20);
    await sleep(500);
    const protocolErr = c.error && !["shell_exit", "input_rate"].includes(c.error);
    record("input-abuse", "binary garbage input: no broker/protocol error", !protocolErr ? "PASS" : "FAIL", c.error);
    if (!c.closed) {
      await c.type("\x03\x03\n", 10, 0);
      record("input-abuse", "binary garbage input: shell responsive afterwards (after Ctrl-C)", (await responsive(c)) ? "PASS" : "FAIL", c.output.slice(-200));
    }
    c.socket.destroy();
    // 3. Input flood beyond the token bucket must end the session quickly.
    c = env.connect(); c.open(account("inflood")); await until(() => c.opened, bounds.bootS * 1000);
    const t0 = Date.now(); await c.type(Buffer.alloc(240_000, 0x61), 16_000, 0);
    record("input-abuse", `240 KB input burst ends the session with input_rate (${Date.now() - t0} ms)`, await until(() => c.error === "input_rate", 5000) ? "PASS" : "FAIL", c.endReason);
    c.socket.destroy();
    // 4. Resize storm with valid sizes: must not wedge the agent or the PTY.
    c = env.connect(); c.open(account("resize")); await until(() => c.opened, bounds.bootS * 1000);
    for (let i = 0; i < 3000; i++) c.send(FrameType.Resize, resizePayload(1 + (i * 37) % 500, 1 + (i * 13) % 200));
    c.send(FrameType.Resize, resizePayload(100, 30));
    record("input-abuse", "3000 resize frames: session open and shell responsive", !c.closed && (await responsive(c, 15_000)) ? "PASS" : "FAIL", c.endReason ?? c.output.slice(-200));
    c.socket.destroy();
    // 5. Protocol abuse from the client side, each on a live session.
    const abuses: [string, (c: Client) => void][] = [
      ["unknown frame type 0x7f", (k) => k.send(0x7f, Buffer.from("x"))],
      ["READY frame from the client", (k) => k.send(FrameType.Ready)],
      ["EXIT frame from the client", (k) => k.send(FrameType.Exit, Buffer.alloc(4))],
      ["second OPEN on a live session", (k) => k.open(account("again"))],
      ["resize with 0 columns", (k) => k.send(FrameType.Resize, resizePayload(0, 10))],
      ["resize with 501 columns", (k) => k.send(FrameType.Resize, resizePayload(501, 10))],
      ["resize with a 3-byte payload", (k) => k.send(FrameType.Resize, Buffer.alloc(3))],
      ["oversize length header", (k) => k.socket.write(Buffer.from([FrameType.Data, 0x7f, 0xff, 0xff, 0xff]))],
    ];
    for (const [name, act] of abuses) {
      c = env.connect(); c.open(account("abuse")); await until(() => c.opened, bounds.bootS * 1000);
      act(c);
      record("input-abuse", `${name}: connection ended (${c.error ?? "closed"})`, (await until(() => c.closed, 5000)) ? "PASS" : "FAIL", "session survived a protocol violation");
      c.socket.destroy();
    }
  });
}

async function rapidReconnect() {
  const max = 3;
  await scoped("rapid-reconnect", { maxSessions: max, openMs: 3000, idleMs: 120_000 }, async (env, sampler) => {
    const fds0 = socketMode ? 0 : (await readdir("/proc/self/fd")).length;
    // A. 30 clients: OPEN then vanish after 0-300 ms, ten at a time. Each leaves a VM that must be destroyed.
    for (let round = 0; round < 3; round++) {
      await Promise.all(Array.from({ length: 10 }, async () => { const c = env.connect(); c.open(account("flap")); await sleep(Math.random() * 300); c.socket.destroy(); }));
      await sleep(200);
    }
    // B. One account hammering OPEN/close 100 times back to back: never more than one VM at once for it.
    let busy = 0;
    for (let i = 0; i < 100; i++) { const c = env.connect(); c.open("h-sameaccount-0000000000"); await sleep(15); if (c.error === "account_busy") busy++; c.socket.destroy(); }
    record("rapid-reconnect", `same-account OPEN storm: ${busy} refused as account_busy, rest raced the teardown`, "INFO");
    // C. 60 sockets that connect and say nothing: all dropped by the open timeout, no fd growth.
    const mutes = Array.from({ length: 60 }, () => env.connect());
    const dropped = await until(() => mutes.every((m) => m.closed), 10_000);
    record("rapid-reconnect", "60 silent connections are all dropped by the open timeout", dropped ? "PASS" : "FAIL", `${mutes.filter((m) => !m.closed).length} still open`);
    mutes.forEach((m) => m.socket.destroy());
    // D. Five distinct accounts at once against a cap of 3.
    const five = Array.from({ length: 5 }, (_, i) => { const c = env.connect(); c.open(account(`five${i}`)); return c; });
    await until(() => five.filter((c) => c.opened).length + five.filter((c) => c.error === "busy").length === 5, bounds.bootS * 1000);
    const nOpen = five.filter((c) => c.opened).length, nBusy = five.filter((c) => c.error === "busy").length;
    record("rapid-reconnect", `5 simultaneous accounts against cap ${max}: ${nOpen} admitted, ${nBusy} busy`, nOpen <= max && nOpen + nBusy === 5 ? "PASS" : "FAIL", { nOpen, nBusy });
    five.forEach((c) => c.socket.destroy());
    await until(async () => (await env.reserved()) === 0, bounds.teardownS * 1000, 100);
    record("rapid-reconnect", `never more than ${max} VMs alive at once (saw ${sampler.maxVms})`, sampler.maxVms <= max ? "PASS" : "FAIL", `${sampler.maxVms} VM cgroups were alive simultaneously`);
    if (!socketMode) {
      await sleep(500);
      const fds1 = (await readdir("/proc/self/fd")).length;
      record("rapid-reconnect", `file descriptors back near baseline (${fds0} -> ${fds1})`, fds1 - fds0 <= bounds.fdGrowth ? "PASS" : "FAIL", `grew by ${fds1 - fds0}: sockets leaked`);
    }
    return { vms: max };
  });
}

async function jailerInspect() {
  await scoped("jailer-inspect", {}, async (env) => {
    const c = env.connect(); c.open(account("inspect")); await until(() => c.opened, bounds.bootS * 1000);
    await c.type("sleep 1\n");
    const script = path.join(here, "inspect-jailer.sh");
    const out = await new Promise<{ code: number; text: string }>((resolve) => {
      execFile("bash", [script], { env: { ...process.env, FIRECRACKER: cfg.firecracker, SHELL_CHROOT_BASE: cfg.chrootBase, SHELL_VM_UID: String(cfg.uid), SHELL_CGROUP_ROOT: cfg.cgroupRoot, BM_CPU_MAX: cfg.cpuMax, BM_MEM_MAX: String(cfg.memMaxBytes), BM_PIDS_MAX: String(cfg.pidsMax) }, maxBuffer: 4 * MiB, timeout: 60_000 },
        (error, stdout, stderr) => resolve({ code: error ? ((error as { code?: number }).code ?? 1) : 0, text: stdout + stderr }));
    });
    for (const line of out.text.split("\n")) {
      if (line.startsWith("FAIL")) record("jailer-inspect", line.slice(6).trim(), "FAIL");
      else if (line.startsWith("PASS")) record("jailer-inspect", line.slice(6).trim(), "PASS");
      else if (line.startsWith("INFO")) record("jailer-inspect", line.slice(6).trim(), "INFO");
    }
    record("jailer-inspect", "inspect-jailer.sh exit status 0", out.code === 0 ? "PASS" : "FAIL", `exit ${out.code}`);
    c.socket.destroy();
  });
}

/** Real service in a child process: SIGKILL it with a live VM, then prove the next start reaps the orphan; SIGTERM leaves nothing. */
async function brokerCrashReap() {
  const test = "broker-crash-reap";
  const base = await snapshot();
  const sock = path.join(workDir, `svc-${rand()}.sock`);
  const startService = async () => {
    const child: ChildProcess = spawn(process.execPath, ["--import", "tsx", path.join(repoRoot, "scripts/shell-broker.mts")], {
      cwd: repoRoot, stdio: ["ignore", "pipe", "pipe"],
      env: { ...process.env, SHELL_BROKER_SOCKET: sock, SHELL_MODE: "jailed", SHELL_MAX_SESSIONS: "2", FIRECRACKER: cfg.firecracker, JAILER: cfg.jailer, SHELL_KERNEL: cfg.kernel, SHELL_ROOTFS: cfg.rootfs, SHELL_CHROOT_BASE: cfg.chrootBase, SHELL_VM_UID: String(cfg.uid) },
    });
    let log = "";
    child.stdout?.on("data", (d) => (log += d)); child.stderr?.on("data", (d) => (log += d));
    const up = await until(() => log.includes('"listening"'), 40_000);
    return { child, up, log: () => log };
  };
  let svc = await startService();
  record(test, "service starts in jailed mode", svc.up ? "PASS" : "FAIL", svc.log());
  if (!svc.up) { svc.child.kill("SIGKILL"); return; }
  const a = new Client(sock); a.open(account("crashA")); await until(() => a.opened, bounds.bootS * 1000);
  record(test, "session opened through the real service", a.opened ? "PASS" : "FAIL", a.error);
  svc.child.kill("SIGKILL"); // the broker dies without cleaning up
  await sleep(1500);
  const orphans = await vmIds();
  record(test, `after SIGKILL of the broker: ${orphans.length} orphan VM cgroup(s) (expected until the next start reaps them)`, "INFO");
  svc = await startService();
  record(test, "service restarts", svc.up ? "PASS" : "FAIL", svc.log());
  record(test, "restart reaped the stale VM(s)", orphans.length === 0 || /reaped_stale_vms/.test(svc.log()) ? "PASS" : "FAIL", svc.log());
  await assertClean(test, "after reaper", base);
  const b = new Client(sock); b.open(account("crashB")); await until(() => b.opened, bounds.bootS * 1000);
  record(test, "restarted service admits a new session", b.opened ? "PASS" : "FAIL", b.error);
  svc.child.kill("SIGTERM"); // graceful stop with a live session: broker_shutdown must destroy the VM
  const exited = await until(() => svc.child.exitCode !== null || svc.child.signalCode !== null, 30_000);
  record(test, "SIGTERM stops the service", exited ? "PASS" : "FAIL");
  if (!exited) svc.child.kill("SIGKILL");
  await assertClean(test, "after SIGTERM with a live session", base);
  a.socket.destroy(); b.socket.destroy();
}

/** Needs the lab network: a service (--socket) whose VMs have a NIC on a tap in their own netns with default-drop nftables. */
async function netEgress() {
  const test = "net-egress";
  if (process.env.BM_LAB_NETWORK !== "yes") return record(test, "network egress tests", "SKIP", "requires the lab-network setup: set BM_LAB_NETWORK=yes, BM_HOST_ADDRS, and run with --socket against a service whose VMs have a NIC");
  const hostAddrs = process.env.BM_HOST_ADDRS ?? Object.values(os.networkInterfaces()).flat().filter((i) => i && !i.internal).map((i) => i!.address).join(" ");
  const canaryPort = envNum("BM_CANARY_PORT", 39999);
  let tcpHits = 0, udpHits = 0;
  const tcp: NetServer = createTcpServer((s) => { tcpHits++; s.destroy(); });
  const udp = createUdpSocket("udp4"); udp.on("message", () => udpHits++); udp.on("error", () => {});
  await new Promise<void>((r) => tcp.listen(canaryPort, "0.0.0.0", r));
  await new Promise<void>((r) => udp.bind(canaryPort, "0.0.0.0", r));
  const tapBefore = process.env.BM_TAP ? await readNum(`/sys/class/net/${process.env.BM_TAP}/statistics/rx_packets`) : undefined;
  await scoped(test, {}, async (env) => {
    const c = env.connect(); c.open(account("net")); await until(() => c.opened, bounds.bootS * 1000);
    const header = `BM_HOST_ADDRS='${hostAddrs}'\nBM_OTHER_ADDRS='${process.env.BM_OTHER_ADDRS ?? ""}'\nBM_CANARY_PORT=${canaryPort}\nBM_STRICT=${process.env.BM_STRICT ?? 0}`;
    await deliver(c, netScript, header);
    await until(() => /BM_DONE\t42/.test(c.output) || c.closed, 150_000);
    const checks = parseChecks(c.output);
    for (const k of checks) {
      if (k.name === "net.nic" && k.verdict === "SKIP") record(test, "guest has a NIC (BM_LAB_NETWORK=yes demands one)", "FAIL", "no NIC: this broker/service starts VMs without a network interface; configure the lab network first");
      else record(test, `guest: ${k.name}`, k.verdict === "FAIL" ? "FAIL" : k.verdict === "PASS" ? "PASS" : "INFO", k.detail);
    }
    record(test, "egress script ran to completion", /BM_DONE\t42/.test(c.output) ? "PASS" : "FAIL");
    record(test, `host canary saw ${tcpHits} TCP connection(s) and ${udpHits} UDP datagram(s) from the guest (expected 0)`, tcpHits === 0 && udpHits === 0 ? "PASS" : "FAIL", "traffic from the guest reached a listener on the host");
    c.socket.destroy();
  });
  if (process.env.BM_TAP) {
    const after = await readNum(`/sys/class/net/${process.env.BM_TAP}/statistics/rx_packets`);
    record(test, `tap ${process.env.BM_TAP} rx_packets ${tapBefore} -> ${after} (packets the guest put on the wire; review with the nft drop counters)`, "INFO");
  }
  tcp.close(); udp.close();
}

// ------------------------------------------------------------------ plan

type Planned = { id: string; title: string; run: () => Promise<void> };
const plan: Planned[] = [
  ...PAYLOADS.map((p) => ({ id: p.id, title: p.title, run: () => runPayload(p) })),
  { id: "outflood", title: "Output flood: output cap and slow-reader backpressure", run: outputFlood },
  { id: "input-abuse", title: "Long lines, binary garbage, input flood, resize storm, client protocol abuse", run: inputAbuse },
  { id: "rapid-reconnect", title: "Rapid reconnects, same-account storm, silent sockets, cap races", run: rapidReconnect },
  { id: "jailer-inspect", title: "Host-side jailer inspection of a live VM (uid, chroot, seccomp, caps, ns, cgroup, /dev/kvm)", run: jailerInspect },
  { id: "broker-crash-reap", title: "SIGKILL/SIGTERM the real service with a live VM; reaper and cleanup", run: brokerCrashReap },
  { id: "net-egress", title: "Network egress from the guest (requires the lab-network setup)", run: netEgress },
];
const selected = plan.filter((p) => (only.length === 0 || only.includes(p.id)) && !skip.includes(p.id));

if (listOnly || dryRun) {
  console.log(`${selected.length} test(s)${dryRun ? " (dry run: nothing is executed, no VM is started)" : ""}`);
  for (const p of selected) console.log(`  ${p.id.padEnd(18)} ${p.title}`);
  console.log("\nstated bounds:", JSON.stringify({ ...bounds, cpuMax: cfg.cpuMax, memMaxBytes: cfg.memMaxBytes, pidsMax: cfg.pidsMax }));
  if (dryRun) {
    // Static validation of what would be typed into the guest: ASCII only, short lines, no control characters, bounded size.
    let bad = 0;
    for (const [name, text] of [["_lib.sh+net/egress.sh", lib + netScript], ...(await Promise.all(PAYLOADS.map(async (p) => [p.file, lib + (await readFile(path.join(payloadDir, p.file), "utf8"))] as const)))]) {
      const lines = text.split("\n");
      const long = lines.filter((l) => l.length > 1000).length;
      const ctrl = /[^\t\n\x20-\x7e]/.test(text);
      const heredocClash = text.includes("BM_EOF_9c1f");
      const ok = !long && !ctrl && !heredocClash && text.length < 24_000;
      if (!ok) bad++;
      console.log(`${ok ? "PASS" : "FAIL"}  payload ${name}: ${text.length} bytes, ${lines.length} lines${long ? `, ${long} line(s) > 1000 chars` : ""}${ctrl ? ", non-ASCII/control characters" : ""}${heredocClash ? ", contains the heredoc terminator" : ""}`);
    }
    process.exit(bad ? 1 : 0);
  }
  process.exit(0);
}

// ------------------------------------------------------------------ guards, preflight, main

if (process.env.BM_DISPOSABLE_HOST !== GUARD || process.getuid?.() !== 0) {
  console.error(`REFUSING TO RUN: hostile workloads need BM_DISPOSABLE_HOST=${GUARD} and root. Use run.sh on a disposable VM.`);
  process.exit(2);
}
const globalMs = envNum("BM_GLOBAL_TIMEOUT_S", 3600) * 1000;
setTimeout(() => { console.error(`\nGLOBAL TIMEOUT (${globalMs / 1000}s) reached: aborting. run.sh will sweep leftover VMs.`); process.exit(3); }, globalMs).unref();

async function preflight() {
  const t = "preflight";
  const need = (name: string, v: string) => record(t, `${name} is set`, v ? "PASS" : "FAIL", `set ${name}`);
  if (!socketMode) { need("FIRECRACKER", cfg.firecracker); need("JAILER", cfg.jailer); need("SHELL_KERNEL", cfg.kernel); need("SHELL_ROOTFS", cfg.rootfs); }
  else { need("FIRECRACKER (for cgroup paths)", cfg.firecracker); }
  record(t, "cgroup v2 mounted", (await stat(path.join(cfg.cgroupRoot, "cgroup.controllers")).then(() => true, () => false)) ? "PASS" : "FAIL", cfg.cgroupRoot);
  const ctl = (await readText(path.join(cfg.cgroupRoot, "cgroup.controllers"))).split(/\s+/);
  for (const c of ["cpu", "memory", "pids"]) record(t, `cgroup controller '${c}' available`, ctl.includes(c) ? "PASS" : "FAIL", ctl.join(","));
  record(t, "/dev/kvm exists", (await stat("/dev/kvm").then(() => true, () => false)) ? "PASS" : "FAIL");
  const pre = await vmIds();
  record(t, "no VM cgroups from earlier runs", pre.length === 0 ? "PASS" : "FAIL", `${pre.join(",")}: not a clean disposable host; remove them first`);
  record(t, "no mongod or web server on this host (disposable means nothing else lives here)", "INFO", (await Promise.all((await numericPids()).map((p) => readText(`/proc/${p}/comm`)))).filter((c) => /^(mongod|next-server|nginx|node)$/.test(c.trim())).length ? "found mongod/next/nginx/node processes: verify this host is disposable" : "");
  record(t, `host: ${os.release()} ${os.arch()} ${os.cpus().length} cpus ${mib(os.totalmem())} RAM, firecracker=${cfg.firecracker}`, "INFO");
  record(t, "mitigations/SMT (host side channels; informational)", "INFO", `${(await readText("/sys/devices/system/cpu/smt/control")).trim()} | ${(await readText("/sys/devices/system/cpu/vulnerabilities/mds")).trim()}`);
}

console.log("== hostile-workload suite: DISPOSABLE HOST, root ==");
console.log("bounds:", JSON.stringify({ ...bounds, cpuMax: cfg.cpuMax, memMaxBytes: cfg.memMaxBytes, pidsMax: cfg.pidsMax }));
await preflight();
if (failures === 0) {
  const finalBase = await snapshot();
  for (const p of selected) {
    console.log(`\n### ${p.id}: ${p.title}`);
    const t0 = Date.now();
    try { await p.run(); } catch (error) { record(p.id, "test crashed", "FAIL", String((error as Error)?.stack ?? error)); }
    console.log(`    (${p.id} took ${Math.round((Date.now() - t0) / 1000)}s)`);
  }
  console.log("\n### final");
  await assertClean("final", "end of suite", finalBase);
  const finalEnv = await makeEnv({});
  await healthy("final", finalEnv);
  await finalEnv.close().catch(() => {});
} else {
  console.log("preflight failed: not running hostile workloads");
}
if (workDir) await rm(workDir, { recursive: true, force: true }).catch(() => {});
const outPath = process.env.BM_RESULTS_JSON;
if (outPath) await writeFile(outPath, JSON.stringify({ at: new Date().toISOString(), host: os.release(), bounds, failures, skips, results }, null, 1)).catch(() => {});
const counts = results.reduce<Record<string, number>>((m, r) => ((m[r.verdict] = (m[r.verdict] ?? 0) + 1), m), {});
console.log(`\n${JSON.stringify(counts)}`);
console.log(failures ? `${failures} check(s) FAILED${skips && strict ? " (including SKIPs under --strict)" : ""}` : `all checks passed${skips ? ` (${skips} skipped: see SKIP lines; not a full pass until they run)` : ""}`);
process.exit(failures ? 1 : 0);
