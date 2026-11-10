// Compiles and runs learner code inside a bubblewrap sandbox: no network,
// read-only /usr, a private /tmp and work dir, and CPU/memory/process/output
// limits. Imported only from route handlers and scripts (no "server-only" so
// scripts/verify-curriculum.ts can use it directly).
import { AsyncLocalStorage } from "node:async_hooks";
import { spawn } from "node:child_process";
import { createHash, randomBytes } from "node:crypto";
import type { Writable } from "node:stream";
import { mkdtemp, rm, writeFile, readdir, readFile } from "node:fs/promises";
import { existsSync, readdirSync, statSync } from "node:fs";
import { tmpdir, homedir, userInfo } from "node:os";
import path from "node:path";
import type { Exercise, Language } from "./curriculum/types";
import { runnerResources } from "./runner-resources";
import { VERDICT_PRELUDE, compareTranscripts, parseTranscript, type Transcript } from "./verdict";

const MAX_OUTPUT = 64 * 1024;
const MAX_RECORDS = 4 * 1024 * 1024;
const COMPILE_TIMEOUT_MS = 20_000;
// Go has no precompiled standard library in this toolchain, so the first build in a fresh
// sandbox compiles what it imports (about 3 s wall, 25 s CPU across cores). Give it its own budget.
const GO_COMPILE_TIMEOUT_MS = 60_000;
const RUN_TIMEOUT_MS = 5_000;
// Compiling is the memory-heavy step (about 2 GB of address space per job, more for Go), running is
// light. Two pools, so a job holds a compile slot only while compiling. Defaults fit MemoryMax=6G:
// 2 x 2 GB + 3 x 512 MB = 5.5 GB. See docs/deployment-hardening.md.
const slotCount = (name: string, fallback: number) => {
  const n = Number(process.env[name]);
  return Number.isInteger(n) && n >= 1 && n <= 64 ? n : fallback;
};
const POOLS = {
  compile: { active: 0, max: slotCount("RUNNER_COMPILE_SLOTS", 2), queue: [] as Waiter[] },
  run: { active: 0, max: slotCount("RUNNER_RUN_SLOTS", 3), queue: [] as Waiter[] },
};
// CI may install Go outside /usr (for example, actions/setup-go's tool cache).
// When set, mount that toolchain into the sandbox at /go.
const GO_TOOLCHAIN_DIR = process.env.GO_TOOLCHAIN_DIR;
// A pre-built Go build cache (see warmGoCache and docs/deployment-hardening.md). Each compile gets a
// private copy-on-write overlay of it at /tmp/gocache, so the standard library is not rebuilt per run
// and the seed itself is never written by a learner job. Without it, behaviour is unchanged.
const GO_CACHE_SEED = process.env.GO_CACHE_SEED && existsSync(process.env.GO_CACHE_SEED) ? process.env.GO_CACHE_SEED : null;

function findRustToolchain(): string | null {
  if (process.env.RUST_TOOLCHAIN_DIR) return process.env.RUST_TOOLCHAIN_DIR;
  const base = path.join(homedir(), ".rustup", "toolchains");
  const stable = existsSync(base) ? readdirSync(base).find((d) => d.startsWith("stable-")) : undefined;
  return stable ? path.join(base, stable) : existsSync("/usr/bin/rustc") && existsSync("/usr/lib/rustlib") ? "/usr" : null;
}
const RUST_TC = findRustToolchain();

// RLIMIT_NPROC counts every process owned by the real uid, so the limit is
// "what this user already runs + headroom" to stop fork bombs without
// breaking the rest of the desktop.
let cachedNproc: { at: number; limit: number } | undefined;
function nprocLimit(): number {
  if (cachedNproc && Date.now() - cachedNproc.at < 10_000) return cachedNproc.limit;
  const uid = userInfo().uid;
  let count = 0;
  try {
    for (const entry of readdirSync("/proc")) {
      if (!/^\d+$/.test(entry)) continue;
      try {
        if (statSync(`/proc/${entry}`).uid === uid) count++;
      } catch {}
    }
  } catch {}
  // With per-job cgroups, pids.max is the real fork limit, so the shared rlimit only needs to be a loose backstop;
  // otherwise concurrent compiles (rustc and go start many threads) would draw on one small budget.
  cachedNproc = { at: Date.now(), limit: count + (process.env.RUNNER_CGROUP_ROOT ? 4096 : 48) };
  return cachedNproc.limit;
}

export { RunnerBusyError } from "./runner-protocol";
import { RunnerBusyError } from "./runner-protocol";
import type { RunResult, TestResult, TestCaseResult } from "./runner-types";
export type { RunResult, TestResult, TestCaseResult } from "./runner-types";
const MAX_QUEUED = 24;
const QUEUE_TIMEOUT_MS = 30_000;
type Owner = { key: string; kind: "user" | "ip" };
const requestContext = new AsyncLocalStorage<{ signal: AbortSignal; owner?: Owner }>();
const inFlight = new Map<string, number>();

/** Count queued and running requests per account and network, independently. */
const envCount = (name: string, fallback: number) => {
  const n = Number(process.env[name]);
  return Number.isInteger(n) && n >= 1 && n <= 64 ? n : fallback;
};
/** In-flight requests allowed per key. Signed-in learners are counted per account; an IP key is only used for anonymous traffic, so a classroom behind one address needs more headroom. */
const INFLIGHT_LIMIT = { user: envCount("RUNNER_INFLIGHT_USER", 2), ip: envCount("RUNNER_INFLIGHT_IP", 6) };

export async function withRunnerRequest<T>(keys: string[], signal: AbortSignal, fn: () => Promise<T>, kinds: ("user" | "ip")[] = []): Promise<T> {
  const limitFor = new Map(keys.map((key, i) => [key, INFLIGHT_LIMIT[kinds[i] ?? "user"]]));
  const unique = [...new Set(keys)];
  if (unique.some((key) => (inFlight.get(key) ?? 0) >= (limitFor.get(key) ?? INFLIGHT_LIMIT.user))) throw new RunnerBusyError("You already have runs in progress. Wait for one to finish.");
  signal.throwIfAborted();
  const owner: Owner | undefined = keys[0] ? { key: keys[0], kind: kinds[0] ?? "user" } : undefined;
  for (const key of unique) inFlight.set(key, (inFlight.get(key) ?? 0) + 1);
  try {
    return await requestContext.run({ signal, owner }, fn);
  } finally {
    for (const key of unique) {
      const count = (inFlight.get(key) ?? 1) - 1;
      if (count) inFlight.set(key, count);
      else inFlight.delete(key);
    }
  }
}

type Waiter = { grant: () => void; owner?: Owner };
type Pool = (typeof POOLS)[keyof typeof POOLS];
export const runnerStatus = () => ({
  active: POOLS.compile.active + POOLS.run.active,
  queued: POOLS.compile.queue.length + POOLS.run.queue.length,
  compile: { active: POOLS.compile.active, queued: POOLS.compile.queue.length, slots: POOLS.compile.max },
  run: { active: POOLS.run.active, queued: POOLS.run.queue.length, slots: POOLS.run.max },
});

// Fair scheduling. Signed-in accounts hold at most one compile slot at a time (anonymous IP keys can share a
// network, so they are only balanced, not capped). Free slots go to the waiting owner served least recently,
// not to the oldest request. Every compile, including each `set`-case recompile, is one admission and counts
// against a per-owner budget per minute.
const COMPILE_BUDGET = { limit: envCount("RUNNER_COMPILE_BUDGET", 60), windowMs: 60_000 };
const compileStarts = new Map<string, number[]>();
let serveClock = 0;
const lastServed = new Map<string, number>();
const holds = new Map<string, number>();
const poolKey = (pool: Pool, owner: Owner) => `${pool === POOLS.compile ? "c" : "r"}:${owner.key}`;
const mayHold = (pool: Pool, owner?: Owner) => !owner || pool !== POOLS.compile || owner.kind !== "user" || !holds.get(poolKey(pool, owner));
function chargeCompile(owner?: Owner) {
  if (!owner) return;
  const now = Date.now();
  const recent = (compileStarts.get(owner.key) ?? []).filter((t) => now - t < COMPILE_BUDGET.windowMs);
  if (recent.length >= COMPILE_BUDGET.limit) { compileStarts.set(owner.key, recent); throw new RunnerBusyError("You are compiling too often. Wait a minute and try again."); }
  recent.push(now);
  compileStarts.set(owner.key, recent);
  if (compileStarts.size > 4096) for (const [k, v] of compileStarts) if (v.every((t) => now - t >= COMPILE_BUDGET.windowMs)) compileStarts.delete(k);
}
function take(pool: Pool, owner?: Owner) {
  pool.active++;
  if (!owner) return;
  holds.set(poolKey(pool, owner), (holds.get(poolKey(pool, owner)) ?? 0) + 1);
  lastServed.set(owner.key, ++serveClock);
  if (lastServed.size > 4096) for (const k of lastServed.keys()) { if (lastServed.size <= 2048) break; lastServed.delete(k); }
}
function release(pool: Pool, owner?: Owner) {
  pool.active--;
  if (owner) {
    const k = poolKey(pool, owner);
    const n = (holds.get(k) ?? 1) - 1;
    if (n) holds.set(k, n); else holds.delete(k);
  }
  // Pick the eligible waiter whose owner was served longest ago; ties keep arrival order.
  let best = -1;
  let bestServed = Infinity;
  pool.queue.forEach((w, i) => {
    if (!mayHold(pool, w.owner)) return;
    const served = w.owner ? lastServed.get(w.owner.key) ?? 0 : 0;
    if (served < bestServed) { best = i; bestServed = served; }
  });
  if (best >= 0 && pool.active < pool.max) {
    const [next] = pool.queue.splice(best, 1);
    take(pool, next.owner); // Occupy the slot on the waiter's behalf; new arrivals cannot steal it.
    next.grant();
  }
}
async function withSlot<T>(pool: Pool, fn: () => Promise<T>): Promise<T> {
  const { queue } = pool;
  const store = requestContext.getStore();
  const signal = store?.signal;
  const owner = store?.owner;
  signal?.throwIfAborted();
  if (pool === POOLS.compile) chargeCompile(owner);
  if (pool.active < pool.max && mayHold(pool, owner) && !queue.some((w) => mayHold(pool, w.owner) && (w.owner ? lastServed.get(w.owner.key) ?? 0 : 0) < (owner ? lastServed.get(owner.key) ?? 0 : 0))) take(pool, owner);
  else {
    if (queue.length >= MAX_QUEUED) throw new RunnerBusyError("The code runner is busy.");
    await new Promise<void>((resolve, reject) => {
      const clear = () => { clearTimeout(timer); signal?.removeEventListener("abort", abort); };
      const remove = (error: Error) => {
        const index = queue.indexOf(waiter);
        if (index < 0) return;
        queue.splice(index, 1);
        clear();
        reject(error);
      };
      const abort = () => remove(new DOMException("Run cancelled", "AbortError"));
      const waiter: Waiter = { owner, grant: () => { clear(); resolve(); } };
      const timer = setTimeout(() => remove(new RunnerBusyError("The runner queue timed out.")), QUEUE_TIMEOUT_MS);
      queue.push(waiter);
      signal?.addEventListener("abort", abort, { once: true });
      if (signal?.aborted) abort();
    });
  }
  try {
    signal?.throwIfAborted();
    return await fn();
  } finally {
    release(pool, owner);
  }
}

type Exec = { stdout: string; stderr: string; code: number | null; signal: string | null; timedOut: boolean; truncated: boolean; ms: number; records?: string };
/** `seed`: verdict runs only. The 16 seed bytes are written to fd 4 and fd 5 carries the driver's records back (src/lib/verdict.ts). */
type ExecOpts = { timeoutMs: number; stdin?: string | Buffer;limits: string[]; env?: Record<string, string>; run?: boolean; tmpMb?: number; artifact?: string; goCache?: "overlay" | "seed"; seed?: Buffer };

// Syscalls no learner program or toolchain needs, and which are the usual route from a
// sandboxed process into the host kernel (io_uring, bpf, userfaultfd, perf, ptrace, module
// and namespace tricks). Denied with EPERM rather than killing, so tools fail cleanly.
const DENIED_SYSCALLS = [
  101, // ptrace
  103, // syslog
  163, // acct
  164, // settimeofday
  165, 166, // mount, umount2
  155, 161, // pivot_root, chroot
  167, 168, 169, // swapon, swapoff, reboot
  175, 176, 313, // init_module, delete_module, finit_module
  179, // quotactl
  212, // lookup_dcookie
  227, // clock_settime
  246, 320, // kexec_load, kexec_file_load
  248, 249, 250, // add_key, request_key, keyctl
  272, 308, // unshare, setns
  298, // perf_event_open
  304, // open_by_handle_at
  310, 311, // process_vm_readv, process_vm_writev
  321, // bpf
  323, // userfaultfd
  425, 426, 427, // io_uring_setup, io_uring_enter, io_uring_register
  428, 429, 430, 431, 432, 433, 442, // open_tree, move_mount, fsopen, fsconfig, fsmount, fspick, mount_setattr
  312, 438, // kcmp, pidfd_getfd
  172, 173, 154, // iopl, ioperm, modify_ldt
  305, 447, // clock_adjtime, memfd_secret
];

/** A classic-BPF seccomp program: x86-64 only, the syscalls above return EPERM, everything else is allowed. */
function seccompFilter(): Buffer {
  const n = DENIED_SYSCALLS.length;
  const ins: [number, number, number, number][] = [
    [0x20, 0, 0, 4], // ld arch
    [0x15, 1, 0, 0xc000003e], // jeq x86_64 -> skip kill
    [0x06, 0, 0, 0x80000000], // ret KILL_PROCESS
    [0x20, 0, 0, 0], // ld syscall nr
    [0x35, n + 1, 0, 0x40000000], // jge x32 range -> deny
    ...DENIED_SYSCALLS.map((nr, i): [number, number, number, number] => [0x15, n - i, 0, nr]), // jeq nr -> deny
    [0x06, 0, 0, 0x7fff0000], // ret ALLOW
    [0x06, 0, 0, 0x00050001], // ret ERRNO(EPERM)
  ];
  const buf = Buffer.alloc(ins.length * 8);
  ins.forEach(([code, jt, jf, k], i) => {
    buf.writeUInt16LE(code, i * 8);
    buf.writeUInt8(jt, i * 8 + 2);
    buf.writeUInt8(jf, i * 8 + 3);
    buf.writeUInt32LE(k, i * 8 + 4);
  });
  return buf;
}
const SECCOMP = seccompFilter();

function sandboxArgs(work: string, extraEnv: Record<string, string> = {}, opts: { run?: boolean; tmpMb?: number; artifact?: string; goCache?: "overlay" | "seed" } = {}): string[] {
  const args = [
    "--unshare-all",
    "--unshare-user",
    "--disable-userns",
    "--seccomp", "3",
    "--die-with-parent",
    "--new-session",
    "--ro-bind", "/usr", "/usr",
    "--symlink", "usr/lib", "/lib",
    "--symlink", "usr/lib64", "/lib64",
    "--symlink", "usr/bin", "/bin",
    "--ro-bind", "/etc/ld.so.cache", "/etc/ld.so.cache",
    "--proc", "/proc",
    "--dev", "/dev",
    // Bounded scratch space (these are RAM); /dev/shm would otherwise default to half of RAM.
    "--size", String((opts.tmpMb ?? 256) * MB), "--tmpfs", "/dev/shm",
    "--size", String((opts.tmpMb ?? 256) * MB), "--tmpfs", "/tmp",
    // Running learner code can't write to the work dir, so it can't fill the disk with files.
    ...(opts.artifact ? ["--ro-bind", work, "/input", "--size", String((opts.tmpMb ?? 512) * MB), "--tmpfs", "/work"] : ["--ro-bind", work, "/work"]),
    "--chdir", "/work",
    "--clearenv",
    "--setenv", "PATH", "/usr/bin",
    "--setenv", "HOME", "/work",
    "--setenv", "LANG", "C.UTF-8",
  ];
  if (RUST_TC) args.push("--ro-bind", RUST_TC, RUST_TC);
  if (GO_TOOLCHAIN_DIR) args.push("--dir", "/go", "--ro-bind", GO_TOOLCHAIN_DIR, "/go");
  if (opts.goCache === "overlay" && GO_CACHE_SEED) args.push("--overlay-src", GO_CACHE_SEED, "--tmp-overlay", "/tmp/gocache");
  if (opts.goCache === "seed" && GO_CACHE_SEED) args.push("--bind", GO_CACHE_SEED, "/tmp/gocache");
  for (const [k, v] of Object.entries(extraEnv)) args.push("--setenv", k, v);
  return args;
}

/** Occupy a runner slot for the duration of `fn` under the current request's identity (exported for scheduler checks). */
export const withRunnerSlot = <T,>(pool: "compile" | "run", fn: () => Promise<T>) => withSlot(POOLS[pool], fn);

async function exec(work: string, cmd: string[], opts: ExecOpts): Promise<Exec> {
  // Builds (artifact jobs) take a compile slot, program runs a run slot, each only while the child lives.
  return withSlot(opts.artifact ? POOLS.compile : POOLS.run, () => execUnslotted(work, cmd, opts));
}

async function execUnslotted(work: string, cmd: string[], opts: ExecOpts): Promise<Exec> {
  const resources = await runnerResources(!!opts.artifact);
  try {
    return await new Promise<Exec>((resolve) => {
      const signal = requestContext.getStore()?.signal;
      signal?.throwIfAborted();
      const started = Date.now();
      let settled = false;
      const args = [...sandboxArgs(work, opts.env, { run: opts.run, tmpMb: opts.tmpMb ?? (opts.run ? 32 : undefined), artifact: opts.artifact, goCache: opts.goCache }), "--", "/usr/bin/prlimit", ...opts.limits, "--",
        ...(opts.artifact ? ["sh", "-c", 'cp -a /input/. /work/ && "$@" && cat "$BM_ARTIFACT" >&4', "build", ...cmd] : cmd)];
      if (opts.artifact) {
        const at = args.indexOf("--clearenv") + 1;
        args.splice(at, 0, "--setenv", "BM_ARTIFACT", opts.artifact);
      }
      const extra = opts.artifact ? ["pipe" as const] : opts.seed ? ["pipe" as const, "pipe" as const] : [];
      const child = spawn(resources.command, [...resources.prefix, ...args], { stdio: ["pipe", "pipe", "pipe", "pipe", ...extra] });
      let records = "";
      if (opts.seed && !opts.artifact) {
        const seedPipe = child.stdio[4] as Writable;
        seedPipe.on("error", () => {});
        seedPipe.end(opts.seed);
        ((child.stdio as unknown[])[5] as import("node:stream").Readable).on("data", (chunk: Buffer) => {
          if (records.length + chunk.length > MAX_RECORDS) {
            if (!truncated) { truncated = true; child.kill("SIGKILL"); }
            return;
          }
          records += chunk.toString("latin1");
        });
      }
      const binary: Buffer[] = [];
      let binarySize = 0;
      if (opts.artifact) (child.stdio[4] as import("node:stream").Readable).on("data", (chunk: Buffer) => {
        binarySize += chunk.length;
        if (binarySize <= 64 * MB) binary.push(chunk);
        else child.kill("SIGKILL");
      });
      const seccomp = child.stdio[3] as Writable;
      seccomp.on("error", () => {});
      seccomp.end(SECCOMP);
      let stdout = "";
      let stderr = "";
      let truncated = false;
      let timedOut = false;
      const take = (which: "out" | "err") => (chunk: Buffer) => {
        const total = stdout.length + stderr.length;
        if (total >= MAX_OUTPUT) {
          if (!truncated) {
            truncated = true;
            child.kill("SIGKILL");
          }
          return;
        }
        const s = chunk.toString("utf8").slice(0, MAX_OUTPUT - total);
        if (which === "out") stdout += s;
        else stderr += s;
      };
      child.stdout.on("data", take("out"));
      child.stderr.on("data", take("err"));
      const timer = setTimeout(() => {
        timedOut = true;
        child.kill("SIGKILL");
      }, opts.timeoutMs);
      const abort = () => child.kill("SIGKILL");
      signal?.addEventListener("abort", abort, { once: true });
      if (signal?.aborted) abort();
      child.on("error", (error) => {
        if (settled) return;
        settled = true;
        clearTimeout(timer);
        signal?.removeEventListener("abort", abort);
        stderr += `Sandbox could not start: ${error.message}`;
        resolve({ stdout, stderr, code: 1, signal: null, timedOut, truncated, ms: Date.now() - started });
      });
      child.on("close", async (code, childSignal) => {
        if (settled) return;
        settled = true;
        clearTimeout(timer);
        signal?.removeEventListener("abort", abort);
        if (opts.artifact && code === 0 && binarySize <= 64 * MB) {
          try { await writeFile(path.join(work, opts.artifact), Buffer.concat(binary), { mode: 0o700 }); }
          catch (error) { stderr += String(error); code = 1; }
        }
        resolve({ stdout, stderr, code, signal: childSignal, timedOut, truncated, ms: Date.now() - started, ...(opts.seed ? { records } : {}) });
      });
      child.stdin.on("error", () => {});
      child.stdin.end(opts.stdin ?? "");
    });
  } finally {
    await resources.cleanup();
  }
}

const MB = 1024 * 1024;
const compileLimits = () => [`--as=${2048 * MB}`, `--cpu=30`, `--nproc=${nprocLimit()}`, `--fsize=${64 * MB}`];
const goCompileLimits = () => [`--cpu=120`, `--nproc=${nprocLimit() + 200}`, `--fsize=${256 * MB}`];
// The Go runtime reserves a lot of address space up front, so the address-space cap is looser than for C.
const goRunLimits = () => [`--as=${4096 * MB}`, `--cpu=10`, `--nproc=${nprocLimit() + 64}`, `--fsize=${4 * MB}`, `--nofile=128`, `--core=0`];
const runLimits = () => [`--as=${256 * MB}`, `--cpu=5`, `--nproc=${nprocLimit()}`, `--fsize=${4 * MB}`, `--nofile=64`, `--core=0`];

// Strip the sandbox paths from compiler output so messages read naturally.
const cleanCompilerOutput = (s: string) => s.replaceAll("/work/", "").trim();

const HIDDEN_NOTE = "(a diagnostic inside the hidden tests was hidden)";

/**
 * Compiler diagnostics quote the source line they complain about, and the hidden tests are
 * compiled into the same unit, so a learner could provoke an error in a harness line to read
 * it (docs/security.md L2). In test builds, diagnostics located in harness code are
 * replaced by a one-line note. `learnerLines` is how many lines of main.rs are the learner's
 * (Rust appends the tests to the same file); the other languages name a separate harness file.
 */
export function redactHarnessDiagnostics(output: string, language: Language, learnerLines: number): string {
  if (language === "rust") {
    // A diagnostic can point at harness code from a primary span (`-->`), a secondary span (`:::`)
    // or only through a quoted source line, so every location and every numbered gutter line counts.
    const beyond = (block: string) =>
      [...block.matchAll(/(?:-->|:::)\s*(?:\S*\/)?main\.rs:(\d+)/g)].some((m) => Number(m[1]) > learnerLines) ||
      [...block.matchAll(/^\s*(\d+)\s+\|/gm)].some((m) => Number(m[1]) > learnerLines) ||
      /__bm_tests/.test(block);
    return output
      .split(/\n\s*\n/)
      .map((block) => (beyond(block) ? HIDDEN_NOTE : block))
      .join("\n\n");
  }
  const harnessFile = language === "go" ? "bm_test.go" : "tests.c";
  const names = (line: string) => line.includes(harnessFile);
  const out: string[] = [];
  let hidden = false;
  for (const line of output.split("\n")) {
    const head = line.match(/^(?:vet: |In file included from )?(\S+?\.[a-z]+):(?:\d+|\s+In )/);
    if (head) hidden = head[1].split("/").pop() === harnessFile;
    // A learner-file diagnostic can still name harness lines ("other declaration of ...").
    if (!hidden && !(language === "go" && names(line) && /bm_test\.go:\d+/.test(line))) out.push(line);
    else if (out[out.length - 1] !== HIDDEN_NOTE) out.push(HIDDEN_NOTE);
  }
  return out.join("\n");
}

function signalMessage(signal: string | null, code: number | null): string | null {
  const sig = signal ?? (code !== null && code > 128 ? `signal ${code - 128}` : null);
  if (!sig) return null;
  const known: Record<string, string> = {
    SIGSEGV: "Segmentation fault — your program touched memory it doesn't own (often a bad pointer or reading past the end of an array).",
    SIGABRT: "Aborted — often a failed assertion or the C library detecting heap corruption (e.g. freeing something twice).",
    SIGFPE: "Arithmetic error — most often dividing an integer by zero.",
    SIGXCPU: "CPU time limit exceeded — check for a loop that never ends.",
    SIGKILL: "Killed — the program ran too long or used too much memory.",
    SIGBUS: "Bus error — usually a misaligned or invalid memory access.",
    "signal 11": "Segmentation fault — your program touched memory it doesn't own (often a bad pointer or reading past the end of an array).",
    "signal 6": "Aborted — often a failed assertion or the C library detecting heap corruption.",
  };
  return known[sig] ?? `Your program was stopped by ${sig}.`;
}

// ---------- Source assembly ----------

const C_FLAGS = ["-std=c17", "-Wall", "-Wextra", "-g", "-O0", "-fsanitize=undefined", "-fno-sanitize-recover=undefined", "-fstack-protector-strong"];

// The nonce is not compiled into the binary (reading the test binary back would reveal it). The
// run passes it in BM_NONCE; a .preinit_array hook, which runs before any constructor, copies it
// into a static and overwrites the original string so /proc/self/environ no longer holds it.
// Learner code in the same process could still scan live memory for it (accepted residual,
// docs/security.md L1). Completeness rejects truncated transcripts but is not a trust boundary.
const cTestPrelude = () => {
  // Fresh unguessable identifiers prevent a learner from naming the harness static
  // directly in the same translation unit. The process can still inspect its own memory.
  const suffix = randomBytes(16).toString("hex");
  const nonceVar = `bm_nonce_${suffix}`, takeNonce = `bm_take_${suffix}`, hook = `bm_hook_${suffix}`;
  return `
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
static char ${nonceVar}[40];
static void ${takeNonce}(int argc, char **argv, char **envp) {
  (void)argc; (void)argv;
  for (; envp && *envp; envp++) {
    if (strncmp(*envp, "BM_NONCE=", 9) == 0) {
      strncpy(${nonceVar}, *envp + 9, sizeof ${nonceVar} - 1);
      memset(*envp, 0, strlen(*envp));
      return;
    }
  }
}
__attribute__((section(".preinit_array"), used)) static void (*${hook})(int, char **, char **) = ${takeNonce};
#define CHECK(name, cond, ...) do { \\
  printf("\\n@@RUN %s %s\\n", ${nonceVar}, name); fflush(stdout); \\
  if (cond) { printf("\\n@@PASS %s %s\\n", ${nonceVar}, name); } \\
  else { printf("\\n@@FAIL %s %s\\t", ${nonceVar}, name); printf(__VA_ARGS__); printf("\\n"); } \\
  fflush(stdout); } while (0)
`;
};

function cTestSource(student: string, harness: string, undefs = "") {
  // Learners may keep their own main() for experimenting; rename it so the
  // test harness's main() is the entry point.
  // main is renamed with -Dmain=... on the command line rather than a #define
  // here, so main.c's line numbers and gcc's source excerpts stay exact.
  return `${student}\n${undefs}\n#undef main\n#line 1 "tests.c"\n${cTestPrelude()}\n${harness}\n`;
}

function rustTestSource(student: string, harness: string) {
  return `${student}\n\n#[cfg(test)]\nmod __bm_tests {\n#![allow(unused_imports)]\nuse super::*;\n${harness}\n}\n`;
}

// ---------- Kernel (x86-64 long mode, booted in QEMU) ----------

// Learner code is freestanding C. A hidden boot stub (src/lib/kernel/boot.S) takes the
// machine from Multiboot's 32-bit mode to long mode and calls kmain(); a hidden runtime
// (runtime.c) supplies port I/O, mem*(), printf over the QEMU debug console and a default
// exception reporter. Test results travel on that debug console (port 0xE9), so they
// never mix with the learner's own serial output on COM1 (QEMU's stdout).
const KERNEL_SUPPORT = path.join(process.cwd(), "src", "lib", "kernel");
const KERNEL_CFLAGS = "-m64 -std=gnu17 -ffreestanding -fno-pic -fno-pie -mno-red-zone -mgeneral-regs-only -fno-stack-protector -fcf-protection=none -fno-asynchronous-unwind-tables -nostdlib -Wall -Wextra -g -O0";

const kernelTestPrelude = (nonce: string) => `
#include <stdint.h>
#include <stddef.h>
void bm_printf(const char *fmt, ...);
void *memcpy(void *, const void *, size_t);
void *memset(void *, int, size_t);
int memcmp(const void *, const void *, size_t);
static inline void bm_outb(uint16_t p, uint8_t v) { __asm__ volatile("outb %0, %1" : : "a"(v), "Nd"(p)); }
static inline uint8_t bm_inb(uint16_t p) { uint8_t v; __asm__ volatile("inb %1, %0" : "=a"(v) : "Nd"(p)); return v; }
static inline void bm_outl(uint16_t p, uint32_t v) { __asm__ volatile("outl %0, %1" : : "a"(v), "Nd"(p)); }
static inline uint32_t bm_inl(uint16_t p) { uint32_t v; __asm__ volatile("inl %1, %0" : "=a"(v) : "Nd"(p)); return v; }
#define CHECK(name, cond, ...) do { \\
  bm_printf("\\n@@RUN ${nonce} %s\\n", name); \\
  if (cond) { bm_printf("\\n@@PASS ${nonce} %s\\n", name); } \\
  else { bm_printf("\\n@@FAIL ${nonce} %s\\t", name); bm_printf(__VA_ARGS__); bm_printf("\\n"); } } while (0)
`;

function kernelTestSource(student: string, harness: string, nonce: string, undefs = "") {
  // Same trick as the C harness: the learner may keep a kmain() to experiment, so rename it.
  return `${student}\n${undefs}\n#undef kmain\n#line 1 "tests.c"\n${kernelTestPrelude(nonce)}\n${harness}\n`;
}

const KERNEL_LINK = [
  `gcc ${KERNEL_CFLAGS} -c boot.S runtime.c`,
  `ld -m elf_x86_64 -nostdlib -z max-page-size=4096 -T link.ld -o kernel64.elf boot.o runtime.o kernel.o`,
  `objcopy -O elf32-i386 kernel64.elf kernel.elf`,
];

async function writeKernelSupport(dir: string) {
  for (const f of ["boot.S", "runtime.c", "link.ld"]) await writeFile(path.join(/*turbopackIgnore: true*/ dir, f), await readFile(path.join(/*turbopackIgnore: true*/ KERNEL_SUPPORT, f)));
}

/** Run mode only (no hidden code). Test builds go through compileKernelTest. */
async function compileKernel(dir: string, source: string) {
  await writeKernelSupport(dir);
  await writeFile(path.join(dir, "kernel.c"), source);
  const script = [`gcc ${KERNEL_CFLAGS} -c kernel.c`, ...KERNEL_LINK].join(" && ");
  return exec(dir, ["sh", "-c", script], { timeoutMs: COMPILE_TIMEOUT_MS, limits: compileLimits(), artifact: "kernel.elf" });
}

/** Test mode: assembles the scanned combined source (compileToAssembly) from stdin, so no source file sits beside it. */
async function compileKernelTest(dir: string, built: Assembled) {
  await writeKernelSupport(dir);
  const script = [`gcc -m64 -c -x assembler - -o kernel.o`, ...KERNEL_LINK].join(" && ");
  const r = await exec(dir, ["sh", "-c", script], { stdin: built.asm, timeoutMs: COMPILE_TIMEOUT_MS, limits: compileLimits(), artifact: "kernel.elf" });
  return { ...r, stderr: built.step.stderr + r.stderr, stdout: built.step.stdout + r.stdout };
}

// QEMU needs a lot of address space, so no --as limit here; CPU time, process count, file
// size and the wall-clock timeout still bound it. No network, no disk, no display.
const qemuLimits = () => [`--cpu=10`, `--nproc=${nprocLimit()}`, `--fsize=${4 * MB}`, `--nofile=256`, `--core=0`];

type KernelRun = Exec & { debug: string; exception: string | null; exitStatus: number | null };

async function bootKernel(dir: string): Promise<KernelRun> {
  const qemu = [
    "qemu-system-x86_64", "-machine", "pc,accel=tcg", "-cpu", "qemu64", "-m", "64",
    "-nodefaults", "-vga", "std", "-display", "none", "-no-reboot", "-monitor", "none", "-net", "none",
    "-kernel", "kernel.elf", "-serial", "file:/tmp/serial.log", "-debugcon", "stdio",
    "-device", "isa-debug-exit,iobase=0xf4,iosize=0x04",
  ];
  // QEMU's stdio backend carries debug results; serial output stays on bounded tmpfs
  // and is copied to stderr after exit. The kernel image remains read-only.
  const r = await exec(dir, ["sh", "-c", '"$@"; status=$?; cat /tmp/serial.log >&2; exit "$status"', "boot", ...qemu], { tmpMb: 256, timeoutMs: RUN_TIMEOUT_MS, limits: qemuLimits() });
  const debug = r.stdout.slice(0, MAX_OUTPUT);
  const exc = debug.match(/^@@EXC (.*)\trip=(0x[0-9a-f]+) err=(0x[0-9a-f]+) cr2=(0x[0-9a-f]+)$/m);
  const exception = exc ? `The CPU raised ${exc[1]} at instruction address ${exc[2]}${exc[1].includes("page fault") ? ` while touching address ${exc[4]}` : ""}.` : null;
  return { ...r, stdout: r.stderr, debug, exception, exitStatus: r.code };
}

/** Why a finished boot counts as a crash, or null if the kernel returned from kmain normally. */
function kernelCrash(r: KernelRun): string | null {
  if (r.timedOut) return `Your kernel was still running after ${RUN_TIMEOUT_MS / 1000} seconds and was stopped. A kernel never exits on its own, so this is normal for a hand-written main loop, but a test that hangs usually means an endless wait for hardware.`;
  if (r.truncated) return "Output was cut off at 64 KB.";
  if (r.exception) return r.exception;
  if (r.exitStatus === 1) return null;
  return "The virtual machine reset itself. That's a triple fault: an exception happened and the CPU could not even run an exception handler (usually a bad GDT, IDT, page table or stack).";
}

// ---------- Public API ----------


async function withWorkdir<T>(fn: (dir: string) => Promise<T>): Promise<T> {
  const dir = await mkdtemp(path.join(tmpdir(), "bm-run-"));
  try {
    return await fn(dir);
  } finally {
    await rm(dir, { recursive: true, force: true });
  }
}

// Pipes make stdout block-buffered, so output printed before a crash would be
// lost. Line-buffer it, the way a terminal would.
const C_STDIO_INIT = `#include <stdio.h>
__attribute__((constructor)) static void __bm_init(void) { setvbuf(stdout, NULL, _IOLBF, 0); }
`;

// Assembly: the learner's file is GNU as with `.intel_syntax noprefix`, linked by gcc.
// No UBSan (nothing to instrument); -no-pie so absolute symbol references work;
// --noexecstack silences the missing .note.GNU-stack linker warning. In test mode
// the hidden C harness (tests.c, with the CHECK prelude) supplies main().
async function compileAsm(dir: string, source: string, harness: string | null) {
  const asmFlags = ["-std=c17", "-Wall", "-Wextra", "-g", "-O0", "-Wa,--noexecstack"];
  await writeFile(path.join(dir, "bm_init.c"), C_STDIO_INIT);
  if (harness === null) {
    await writeFile(path.join(dir, "starter.s"), source);
    return exec(dir, ["gcc", ...asmFlags, "-no-pie", "starter.s", "bm_init.c", "-o", "prog"], { timeoutMs: COMPILE_TIMEOUT_MS, limits: compileLimits(), artifact: "prog" });
  }
  // Test mode builds in three steps so the learner's assembly is only ever read by `as` in a
  // directory that holds nothing but starter.s: the hidden harness is compiled first, to a single
  // relocatable object, and is not on disk when .incbin/.include (in any spelling) could reach it.
  await writeFile(path.join(dir, "tests.c"), `${cTestPrelude()}\n#line 1 "tests.c"\n${harness}\n`);
  const flags = asmFlags.join(" ");
  const built = await exec(dir, ["sh", "-c", `gcc ${flags} -c bm_init.c -o bm_init.o && gcc ${flags} -c tests.c -o tests.o && ld -r bm_init.o tests.o -o harness.o`], { timeoutMs: COMPILE_TIMEOUT_MS, limits: compileLimits(), artifact: "harness.o" });
  if (built.code !== 0) return built;
  const assembled = await withWorkdir(async (asmDir) => {
    await writeFile(path.join(asmDir, "starter.s"), source);
    const r = await exec(asmDir, ["gcc", "-Wa,--noexecstack", "-c", "starter.s", "-o", "starter.o"], { timeoutMs: COMPILE_TIMEOUT_MS, limits: compileLimits(), artifact: "starter.o" });
    return { r, object: r.code === 0 ? await readFile(path.join(asmDir, "starter.o")) : null };
  });
  if (!assembled.object) return assembled.r;
  await writeFile(path.join(dir, "starter.o"), assembled.object);
  for (const f of ["tests.c", "bm_init.c", "bm_init.o", "tests.o"]) await rm(path.join(/*turbopackIgnore: true*/ dir, f), { force: true });
  const linked = await exec(dir, ["gcc", "-no-pie", "starter.o", "harness.o", "-o", "prog"], { timeoutMs: COMPILE_TIMEOUT_MS, limits: compileLimits(), artifact: "prog" });
  return { ...linked, stderr: built.stderr + assembled.r.stderr + linked.stderr, stdout: built.stdout + assembled.r.stdout + linked.stdout };
}

// Go: the learner's file is main.go (package main) in a throwaway module. Tests are a hidden
// bm_test.go in the same package, built with `go test -c` and run with -test.v. No network,
// no cgo; the build cache lives in the sandbox's private /tmp.
const GO_ENV = {
  ...(GO_TOOLCHAIN_DIR ? { GOROOT: "/go" } : {}),
  GOCACHE: "/tmp/gocache",
  GOPATH: "/tmp/gopath",
  GOFLAGS: "-mod=mod -buildvcs=false",
  GOPROXY: "off",
  GOTOOLCHAIN: "local",
  GO111MODULE: "on",
  GOTELEMETRY: "off",
  CGO_ENABLED: "0",
};

async function compileGo(dir: string, source: string, harness: string | null) {
  await writeFile(path.join(dir, "go.mod"), "module bm\n\ngo 1.24\n");
  await writeFile(path.join(dir, "main.go"), source);
  const cmd = [GO_TOOLCHAIN_DIR ? "/go/bin/go" : "go"];
  if (harness !== null) {
    await writeFile(path.join(dir, "bm_test.go"), harness);
    cmd.push("test", "-c", "-vet=off", "-o", "prog");
  } else cmd.push("build", "-o", "prog", ".");
  return exec(dir, cmd, { timeoutMs: GO_COMPILE_TIMEOUT_MS, limits: goCompileLimits(), env: GO_ENV, tmpMb: 1024, artifact: "prog", goCache: "overlay" });
}

/**
 * Fills GO_CACHE_SEED with the compiled standard library, using the same sandbox layout, flags and
 * GOROOT as learner compiles (cache keys depend on them). Run at deploy time by scripts/warm-go-cache.mts.
 */
export async function warmGoCache(): Promise<Exec> {
  if (!GO_CACHE_SEED) throw new Error("Set GO_CACHE_SEED to an existing, writable directory first.");
  return withWorkdir(async (dir) => {
    await writeFile(path.join(dir, "go.mod"), "module bm\n\ngo 1.24\n");
    await writeFile(path.join(dir, "main.go"), "package main\n\nfunc main() {}\n");
    const cmd = [GO_TOOLCHAIN_DIR ? "/go/bin/go" : "go", "build", "std"];
    return exec(dir, cmd, { timeoutMs: 10 * 60_000, limits: goCompileLimits(), env: GO_ENV, tmpMb: 1024, goCache: "seed" });
  });
}

async function compile(dir: string, language: Language, source: string, opts: { test?: boolean; harness?: string } = {}) {
  if (language === "kernel") return compileKernel(dir, source);
  if (language === "go") return compileGo(dir, source, opts.test ? (opts.harness ?? "") : null);
  if (language === "asm") return compileAsm(dir, source, opts.test ? (opts.harness ?? "") : null);
  if (language === "c") {
    await writeFile(path.join(dir, "main.c"), source);
    await writeFile(path.join(dir, "bm_init.c"), C_STDIO_INIT);
    // Run and stdout modes only (no hidden code). Harness builds go through compileCTest.
    return exec(dir, ["gcc", ...C_FLAGS, "main.c", "bm_init.c", "-o", "prog", "-lm"], { timeoutMs: COMPILE_TIMEOUT_MS, limits: compileLimits(), artifact: "prog" });
  }
  if (!RUST_TC) throw new Error("Rust toolchain not found. Install it with rustup or set RUST_TOOLCHAIN_DIR.");
  await writeFile(path.join(dir, "main.rs"), source);
  // The sandbox intentionally hides most of /etc. On Ubuntu, `cc` can resolve
  // through /etc/alternatives and appear missing even though GCC is installed.
  // Use the concrete compiler path, which is already available in the read-only /usr bind.
  const args = [`${RUST_TC}/bin/rustc`, "--edition", "2021", "-C", "opt-level=0", "-C", "linker=/usr/bin/gcc", "--color", "never", "main.rs", "-o", "prog"];
  // lld otherwise sizes its worker pool from the host CPU count; under the
  // compile address-space limit that can intermittently abort even tiny builds.
  if (existsSync("/usr/bin/ld.lld")) args.push("-C", "link-arg=-fuse-ld=lld", "-C", "link-arg=-Wl,--threads=1");
  if (opts.test) args.push("--test", "-A", "dead_code");
  return exec(dir, args, { timeoutMs: COMPILE_TIMEOUT_MS, limits: compileLimits(), artifact: "prog" });
}

const runEnv = { UBSAN_OPTIONS: "print_stacktrace=0", RUST_BACKTRACE: "0" };

function undefinedBehaviour(stderr: string): string | null {
  const line = stderr.split("\n").find((l) => l.includes("runtime error:"));
  return line ? `Undefined behaviour detected at ${cleanCompilerOutput(line)}` : null;
}

/** Compile and run the learner's own program (their main). */
export function runProgram(language: Language, code: string, stdin = ""): Promise<RunResult> {
  return (
    withWorkdir(async (dir) => {
      const c = await compile(dir, language, code);
      const compileOutput = cleanCompilerOutput(c.stderr + c.stdout);
      if (c.code !== 0) {
        const noMain = /undefined reference to `k?main'|`main` function not found|function main is undeclared/.test(compileOutput);
        const noMainText =
          language === "kernel"
            ? "There's no kmain() function to boot. Your kernel starts there. Add one to experiment, or press Test to check your solution."
            : language === "asm"
            ? "There's no `main` label to run. Your functions are called by the hidden tests, so press Test to check them (or add a `.globl main` entry point to experiment)."
            : "There's no main() function to run. Add one to experiment, or press Test to check your solution.";
        return {
          ok: false,
          phase: "compile",
          compileOutput,
          stdout: "",
          stderr: "",
          exitCode: c.code,
          ...(c.timedOut ? { timedOut: true } : {}),
          error: noMain
            ? noMainText
            : c.timedOut
              ? "Compilation took too long."
              : "Your code didn't compile. Read the first error carefully — later errors are often caused by the first.",
          durationMs: c.ms,
        };
      }
      if (language === "kernel") {
        const k = await bootKernel(dir);
        // A kernel parked in a final hlt loop is normal, so a timeout alone is not a failure here.
        const crash = k.timedOut ? null : kernelCrash(k);
        return { ok: !crash, phase: "run", compileOutput, stdout: k.stdout, stderr: k.debug.trim(), exitCode: k.exitStatus === 1 ? 0 : k.exitStatus, error: crash, durationMs: c.ms + k.ms };
      }
      const r = await exec(dir, ["./prog"], { run: true, timeoutMs: RUN_TIMEOUT_MS, stdin, limits: language === "go" ? goRunLimits() : runLimits(), env: runEnv });
      const error = r.timedOut
        ? `Your program ran for more than ${RUN_TIMEOUT_MS / 1000} seconds and was stopped. Is there a loop that never ends, or is it waiting for input?`
        : r.truncated
          ? "Output was cut off at 64 KB — your program printed a lot (maybe an infinite loop?)."
          : (signalMessage(r.signal, r.code) ?? undefinedBehaviour(r.stderr));
      return { ok: !error && r.code === 0, phase: "run", compileOutput, stdout: r.stdout, stderr: r.stderr, exitCode: r.code, error, ...(r.timedOut ? { timedOut: true } : {}), durationMs: c.ms + r.ms };
    })
  );
}

/**
 * Test binaries run with only themselves in the work dir, so learner code can't read the
 * hidden test sources at runtime. (It can still embed them at compile time; see docs/security.md L2.)
 */
async function keepOnlyProgram(dir: string) {
  for (const f of await readdir(dir)) if (f !== "prog") await rm(path.join(dir, f), { recursive: true, force: true });
}


// ---------- Shared translation unit guard (C and kernel) ----------

// C and kernel tests are appended to the learner's file, so anything that can read or rewrite the
// rest of that file defeats them. The plain-text TAMPER_PATTERNS miss spellings built at compile
// time (asm(".inc" "bin \"kernel.c\""), `#define F "main.c"` then `#include F`), so the learner's
// file is also preprocessed and compiled ALONE in an empty sandbox directory:
//  - any include that actually opens a file fails (nothing else is there), and the end-of-file
//    sentinel below errors if the file is entered a second time (a self-include);
//  - the assembly gcc emits is scanned for directives that read files or generate directives;
//  - every macro the learner's own text defines or changes is #undef'd before the harness, so
//    `#define if(c) if(1)` can't rewrite the tests. Macros from system headers are restored to
//    their header definitions. Names the lesson's own starter defines (COM1, ...) are left alone because
//    harnesses use them.
// This standalone build can differ from the real one, so the real combined build is checked again
// (compileToAssembly below); that second check is what stops directives gated on the difference.
// Residual: the harness still shares a translation unit (docs/security.md L1/L2); the proper fix is
// compiling it as a separate object, which struct-testing C harnesses would need type declarations for.
const SENTINEL = "\n#ifdef BM_PREFLIGHT_END\n#error bm-self-include\n#endif\n#define BM_PREFLIGHT_END\n";
const ASM_DIRECTIVES = /(?:^|[;:\n])[ \t]*\.(?:incbin|include|macro|altmacro|irpc?|rept|reptc?|init_array|preinit_array|fini_array|ctors|dtors)\b/i;
const FEATURE_MACRO = /^[ \t]*#[ \t]*define[ \t]+(_GNU_SOURCE|_DEFAULT_SOURCE|_POSIX_C_SOURCE|_XOPEN_SOURCE|_ISOC11_SOURCE|__STDC_WANT_LIB_EXT1__)\b.*$/gm;
const ANGLE_INCLUDE = /^[ \t]*#[ \t]*include[ \t]*<([\w./+-]+)>/gm;

type SourceGuard = { ok: true; undefs: string } | { ok: false; result: TestResult };

const macroTable = (text: string) => {
  const table = new Map<string, string>();
  for (const line of text.split("\n")) {
    const m = /^#define ([A-Za-z_]\w*)/.exec(line);
    if (m) table.set(m[1], line);
  }
  return table;
};

const macroNames = (text: string) => [...text.matchAll(/^[ \t]*#[ \t]*define[ \t]+([A-Za-z_]\w*)/gm)].map((m) => m[1]);

async function guardSharedSource(language: "c" | "kernel", code: string, starter: string): Promise<SourceGuard> {
  const reject = (error: string, compileOutput = "", compiled = true): SourceGuard => ({ ok: false, result: { compiled, compileOutput, results: [], allPassed: false, output: "", error } });
  const file = language === "kernel" ? "kernel.c" : "main.c";
  const rename = language === "kernel" ? "-Dkmain=__bm_student_kmain" : "-Dmain=__bm_student_main";
  const flags = language === "kernel" ? `${KERNEL_CFLAGS} ${rename}` : `-std=c17 ${rename}`;
  const baseHeaders = language === "kernel" ? ["stdint.h", "stddef.h"] : ["stdio.h", "string.h", "stdlib.h", "stdint.h", "stddef.h"];
  const headers = [...new Set([...baseHeaders, ...[...code.matchAll(ANGLE_INCLUDE)].map((m) => m[1])])];
  const base = [...(code.match(FEATURE_MACRO) ?? []), ...headers.map((h) => `#include <${h}>`)].join("\n") + "\n";
  const out = await withWorkdir(async (dir) => {
    await writeFile(path.join(dir, file), code + SENTINEL);
    await writeFile(path.join(dir, "bm_base.c"), base);
    const script = [
      `echo '@@BM-1' > out`,
      `gcc ${flags} -dM -E ${file} >> out`,
      `echo '@@BM-2' >> out`,
      `{ gcc ${flags} -dM -E bm_base.c >> out || true; }`,
      `echo '@@BM-3' >> out`,
      `gcc ${flags} -S ${file} -o - >> out`,
    ].join(" && ");
    const r = await exec(dir, ["sh", "-c", script], { timeoutMs: COMPILE_TIMEOUT_MS, limits: compileLimits(), artifact: "out" });
    return { r, text: r.code === 0 ? await readFile(path.join(dir, "out"), "utf8") : "" };
  });
  const diagnostics = cleanCompilerOutput(out.r.stderr + out.r.stdout);
  if (/bm-self-include/.test(diagnostics)) return reject(HARNESS_REACH_MESSAGE);
  if (out.r.code !== 0) return reject("Your code didn't compile.", diagnostics, false);
  const i1 = out.text.indexOf("@@BM-2\n"), i2 = out.text.indexOf("@@BM-3\n");
  if (!out.text.startsWith("@@BM-1\n") || i1 < 0 || i2 < i1) return reject("Your code didn't compile.", diagnostics, false);
  const macros = out.text.slice(7, i1), baseText = out.text.slice(i1 + 7, i2), asm = out.text.slice(i2 + 7);
  if (ASM_DIRECTIVES.test(asm)) return reject(HARNESS_REACH_MESSAGE);
  // Harnesses may rely on a macro the starter itself defines (COM1, a size constant), so those names
  // stay as the learner left them; every other macro the learner's text defines or changes is removed.
  const promised = new Set(macroNames(starter));
  const mine = macroTable(macros), trusted = macroTable(baseText);
  const lines: string[] = [];
  for (const [name, line] of mine) {
    if (name === "BM_PREFLIGHT_END" || promised.has(name) || trusted.get(name) === line) continue;
    lines.push(`#undef ${name}`);
    const original = trusted.get(name);
    if (original) lines.push(original);
  }
  return { ok: true, undefs: lines.join("\n") };
}

// ---------- Combined test build (C and kernel) ----------

// The guard above sees the learner's file alone, so anything gated on a difference between that
// build and the real one (`#ifdef __SSP_STRONG__`, a file that only the real directory holds) was
// invisible to it. The real build is therefore split in two:
//  1. the combined source (learner + harness, real flags and renames) is compiled with `gcc -S` in a
//     directory holding only that file; every file the preprocessor opened (-MD) must be the source
//     itself or a system header under /usr, and the emitted assembly is scanned for directives that
//     read files or generate directives;
//  2. that exact assembly is fed to `as` on stdin and linked in a directory that holds no source at
//     all (C: bm_init.c; kernel: boot.S, runtime.c, link.ld), so a directive the scan missed still
//     has no harness file to open.
// What is scanned is byte-for-byte what is assembled, so there is no gate left to split the two.
const ASM_READS = /(?:^|[;:\n]|\*\/)[ \t]*\.(?:incbin|include|macro|altmacro|irpc?|rept|reptc?)\b/i;
const ASM_MARK = "\n@@BM-ASM\n";

type Assembled = { asm: Buffer; step: Exec };
type Assembly = ({ ok: true } & Assembled) | { ok: false; reach: boolean; step: Exec };

/** True if gas could read a file or build a directive from this assembly (also with C comments removed). */
const asmReaches = (asm: string) => ASM_READS.test(asm) || ASM_READS.test(asm.replace(/\/\*[\s\S]*?\*\//g, " "));

/** -MD output: every dependency must be the source file itself or a plain path under /usr. */
function unexpectedDependency(deps: string, file: string): boolean {
  const text = deps.replace(/\\\n/g, " ").trim();
  if (!text.startsWith("bm:")) return true;
  return text.slice(3).trim().split(/\s+/).filter(Boolean).some((dep) =>
    dep !== file && !(dep.startsWith("/usr/") && !/\\|\$|(?:^|\/)\.\.?(?:\/|$)/.test(dep)));
}

async function compileToAssembly(language: "c" | "kernel", source: string): Promise<Assembly> {
  const file = language === "kernel" ? "kernel.c" : "main.c";
  const flags = language === "kernel" ? `${KERNEL_CFLAGS} -Dkmain=__bm_student_kmain` : `${C_FLAGS.join(" ")} -Dmain=__bm_student_main`;
  return withWorkdir(async (work): Promise<Assembly> => {
    await writeFile(path.join(work, file), source);
    const script = `gcc ${flags} -MD -MF bm.d -MT bm -S ${file} -o bm.s && { cat bm.d; printf '${ASM_MARK.replaceAll("\n", "\\n")}'; cat bm.s; } > out`;
    const step = await exec(work, ["sh", "-c", script], { timeoutMs: COMPILE_TIMEOUT_MS, limits: compileLimits(), artifact: "out" });
    if (step.code !== 0) return { ok: false, reach: false, step };
    const out = await readFile(path.join(work, "out"));
    const at = out.indexOf(ASM_MARK, 0, "latin1");
    if (at < 0) return { ok: false, reach: true, step };
    const asm = out.subarray(at + ASM_MARK.length);
    if (unexpectedDependency(out.subarray(0, at).toString("latin1"), file) || asmReaches(asm.toString("latin1"))) return { ok: false, reach: true, step };
    return { ok: true, asm, step };
  });
}

async function compileCTest(dir: string, built: Assembled) {
  await writeFile(path.join(dir, "bm_init.c"), C_STDIO_INIT);
  const script = `gcc -c -x assembler - -o main.o && gcc ${C_FLAGS.join(" ")} -c bm_init.c -o bm_init.o && gcc -fsanitize=undefined main.o bm_init.o -o prog -lm`;
  const r = await exec(dir, ["sh", "-c", script], { stdin: built.asm, timeoutMs: COMPILE_TIMEOUT_MS, limits: compileLimits(), artifact: "prog" });
  return { ...r, stderr: built.step.stderr + r.stderr, stdout: built.step.stdout + r.stdout };
}

const reachResult = (): TestResult => ({ compiled: true, compileOutput: "", results: [], allPassed: false, output: "", error: HARNESS_REACH_MESSAGE });

const HARNESS_REACH_MESSAGE ="Your code reads files or builds assembler directives at compile time, which could reach the hidden tests. Tests can't be checked while it's there. Remove it and run the tests again.";

/** Text with no honest use in a solution: the harness's result protocol, its internals, and load-time hooks. */
const TAMPER_PATTERNS: RegExp[] = [
  /@@\s*(?:RUN|PASS|FAIL|EXC|NONCE)/,
  /---\s+(?:PASS|FAIL):/,
  /__bm_tests|\bbm_test\b|__bm_student/,
  /\.incbin|\.init_array|\.preinit_array|\.fini_array|link_section|__attribute__\s*\(\(\s*(?:constructor|destructor)/,
  /\/proc\/(?:self|\d)/,
  // Ways to pull the hidden tests into the learner's own build (docs/security.md L2).
  /\/\/go:embed|\binclude_(?:str|bytes)!|\binclude!\s*[(\[{]|^\s*\.(?:include|incbin)\b|#\s*include\s*"|#\s*include_next\b/m,
];

// Per-language additions. Spelling variants matter: `include_str !(...)`, macros that forward the
// macro name, GNU as directives in any case, and C23 `#embed` all reach a file the compiler can read.
const TAMPER_PATTERNS_BY_LANGUAGE: Partial<Record<Language, RegExp[]>> = {
  // Any use of the include family, and #[path] module attributes, which can name main.rs.
  rust: [/\binclude(?:_str|_bytes)?\b/, /#\s*!?\s*\[\s*path\b/],
  // GNU as directives are case-insensitive; macros can assemble a directive name from parameters.
  asm: [/\.(?:incbin|include|init_array|preinit_array|fini_array)\b/i, /\.(?:alt)?macro\b/i, /\.irpc?\b/i],
  // __has_include(_next) probes which files sit beside the build, which is how a directive can be
  // emitted only where the harness is present.
  c: [/#\s*embed\b|__has_embed|__has_include/],
  kernel: [/#\s*embed\b|__has_embed|__has_include/],
};

export function harnessTampering(code: string, language: Language): string | null {
  const patterns = [...TAMPER_PATTERNS, ...(TAMPER_PATTERNS_BY_LANGUAGE[language] ?? [])];
  if (!patterns.some((re) => re.test(code))) return null;
  return "Your code contains text that belongs to the test harness (its result lines, internal names, load-time hooks or a read of /proc). Tests can't be checked while it's there. Remove it and run the tests again.";
}

/**
 * Marks every character that sits inside a comment or a string/char literal (C, Rust and Go
 * share `//`, `/* *\/`, `"..."`, `'x'` and Go's raw backtick strings). Lifetimes such as `'a`
 * are not char literals, so a quote only opens one when a closing quote follows promptly.
 */
function scanLiterals(code: string): { mask: boolean[]; comments: boolean[] } {
  const mask = new Array<boolean>(code.length).fill(false);
  const comments = new Array<boolean>(code.length).fill(false);
  const fill = (from: number, to: number, into = mask) => { for (let k = from; k < Math.min(to, code.length); k++) into[k] = true; };
  let i = 0;
  while (i < code.length) {
    const c = code[i], n = code[i + 1];
    if (c === "/" && n === "/") {
      const end = code.indexOf("\n", i);
      const to = end < 0 ? code.length : end;
      fill(i, to); fill(i, to, comments); i = to;
    } else if (c === "/" && n === "*") {
      const end = code.indexOf("*/", i + 2);
      const to = end < 0 ? code.length : end + 2;
      fill(i, to); fill(i, to, comments); i = to;
    } else if (c === "`") {
      const end = code.indexOf("`", i + 1);
      const to = end < 0 ? code.length : end + 1;
      fill(i, to); i = to;
    } else if (c === '"') {
      let j = i + 1;
      while (j < code.length && code[j] !== '"' && code[j] !== "\n") j += code[j] === "\\" ? 2 : 1;
      const to = Math.min(j + 1, code.length);
      fill(i, to); i = to;
    } else if (c === "'") {
      const m = /^'(?:\\(?:x[0-9a-fA-F]{2}|u\{[0-9a-fA-F]+\}|.)|[^'\\\n])'/.exec(code.slice(i, i + 12));
      if (m) { fill(i, i + m[0].length); i += m[0].length; } else i++;
    } else i++;
  }
  return { mask, comments };
}

/**
 * Replace the value in the first declaration of each variable (`int a = 7, b = 8;`, `let n: i32 = 3;`,
 * `x := 3`) that isn't inside a comment or a literal. Null if a variable can't be found.
 */
export function setInitialValues(code: string, set: Record<string, string>): string | null {
  let out = code;
  for (const [name, value] of Object.entries(set)) {
    // The declaration is bounded to 200 characters and searched with comments blanked out first, so a
    // huge comment or one very long line can't make the scan quadratic.
    const re = new RegExp(`(\\b${name}\\b[^=;\\n]{0,200}?:?=\\s*)("[^"\\n]*"|'[^'\\n]*'|[^;\\n,]+)`, "g");
    const { mask, comments } = scanLiterals(out);
    const scan = out.replace(/[^\n]/g, (ch, k: number) => (comments[k] ? " " : ch));
    let hit: RegExpExecArray | null = null;
    for (let m = re.exec(scan); m; m = re.exec(scan)) {
      if (!mask[m.index]) { hit = m; break; }
      re.lastIndex = m.index + 1;
    }
    if (!hit) return null;
    out = out.slice(0, hit.index) + out.slice(hit.index, hit.index + hit[1].length) + value + out.slice(hit.index + hit[0].length);
  }
  return out;
}

const normalizeOut = (s: string) => s.replace(/\r\n/g, "\n").replace(/[ \t]+$/gm, "").trimEnd();

/**
 * Run an exercise's tests against learner code. `seed` fixes the 16-byte seed of a driver
 * lesson (curriculum checks only); otherwise every attempt draws a fresh one.
 */
export function testExercise(exercise: Exercise, code: string, opts: { seed?: Buffer } = {}): Promise<TestResult> {
  return (
    withWorkdir(async (dir) => {
      if (exercise.mode === "stdout") return testStdout(dir, exercise, code);
      const tampering = harnessTampering(code, exercise.language);
      if (tampering) return { compiled: true, compileOutput: "", results: [], allPassed: false, output: "", error: tampering };
      if (exercise.driver && exercise.language === "c") return testVerdict(dir, exercise, code, opts.seed ?? randomBytes(16));
      const result = await runHarness(dir, exercise, code);
      if (!result.allPassed) return result;
      // Every reported result must match what the harness really emits, so one forged PASS
      // followed by exit(0) can't pass a lesson.
      const expected = await expectedTestNames(exercise);
      if (!expected || !sameNames(expected, result.results.map((r) => r.name))) {
        return { ...result, allPassed: false, error: TESTS_INCOMPLETE };
      }
      return result;
    })
  );
}

const TESTS_INCOMPLETE = "The tests did not run to completion: the results don't match the full list of checks for this exercise.";

function runHarness(dir: string, exercise: Exercise, code: string): Promise<TestResult> {
  const nonce = randomBytes(12).toString("hex"); // unique to this run, so printed text can't pose as a result
  if (exercise.language === "kernel") return testKernelHarness(dir, exercise, code, nonce);
  if (exercise.language === "go") return testGoHarness(dir, exercise, code);
  if (exercise.language === "asm") return testAsmHarness(dir, exercise, code, nonce);
  if (exercise.language === "c") return testCHarness(dir, exercise, code, nonce);
  return testRustHarness(dir, exercise, code);
}

const sameNames = (a: string[], b: string[]) => a.length === b.length && a.every((n, i) => n === b[i]);

/**
 * The ordered test names an exercise's harness reports, taken from one run of the reference
 * solution (counting CHECK( calls in the source would miss loops and computed names). Cached
 * per process and keyed by content, so editing a lesson in dev invalidates it. Null if the
 * reference solution does not pass, in which case the lesson is broken and verify:content says so.
 */
const expectedCache = new Map<string, Promise<string[] | null>>();
const MAX_EXPECTED_CACHE = 512;
function expectedTestNames(ex: Exercise): Promise<string[] | null> {
  const key = `${ex.id}:${createHash("sha256").update(ex.harness ?? "").update("\0").update(ex.solution).digest("hex")}`;
  let hit = expectedCache.get(key);
  if (hit) {
    expectedCache.delete(key);
    expectedCache.set(key, hit);
  } else {
    // This shared reference run stays within the caller's occupied runner slot,
    // but must not inherit that learner's cancellation signal. Other requests
    // may be awaiting the same promise after the initiating client disconnects.
    const reference = requestContext.exit(() => withWorkdir(async (dir) => {
      const r = await runHarness(dir, ex, ex.solution);
      return r.allPassed ? r.results.map((t) => t.name) : null;
    }));
    hit = reference.catch(() => null).then((names) => {
      // A transient compiler/resource failure must not disable an exercise for
      // the lifetime of the process. Retain only successful reference results.
      if (names === null && expectedCache.get(key) === hit) expectedCache.delete(key);
      return names;
    });
    expectedCache.set(key, hit);
    while (expectedCache.size > MAX_EXPECTED_CACHE) {
      const oldest = expectedCache.keys().next().value;
      if (oldest === undefined) break;
      expectedCache.delete(oldest);
    }
  }
  return hit;
}

async function testStdout(dir: string, ex: Exercise, code: string): Promise<TestResult> {
  const c = await compile(dir, ex.language, code);
  const compileOutput = cleanCompilerOutput(c.stderr + c.stdout);
  if (c.code !== 0) return { compiled: false, compileOutput, results: [], allPassed: false, output: "", error: "Your code didn't compile." };
  const results: TestCaseResult[] = [];
  let output = "";
  let error: string | null = null;
  let timedOut = false;
  // `prog` is rebuilt in place for each `set` case; a plain case that follows must run the learner's own code again.
  let variantBuilt = false;
  for (const tc of ex.cases ?? []) {
    if (!tc.set && variantBuilt) {
      const again = await compile(dir, ex.language, code);
      variantBuilt = false;
      if (again.code !== 0) {
        results.push({ name: tc.name, passed: false, message: "Your code could not be rebuilt for this check." });
        continue;
      }
    }
    if (tc.set) {
      const changed = setInitialValues(code, tc.set);
      if (changed === null) {
        results.push({ name: tc.name, passed: false, message: `Keep the starter's variables (${Object.keys(tc.set).join(", ")}) and calculate from them, so the program works for other numbers too.` });
        continue;
      }
      variantBuilt = true;
      const cc = await compile(dir, ex.language, changed);
      if (cc.code !== 0) {
        results.push({ name: tc.name, passed: false, message: "The program stopped compiling once the starting numbers changed." });
        continue;
      }
    }
    const r = await exec(dir, ["./prog"], { run: true, timeoutMs: RUN_TIMEOUT_MS, stdin: tc.stdin ?? "", limits: ex.language === "go" ? goRunLimits() : runLimits(), env: runEnv });
    output = r.stdout + r.stderr;
    const crash = r.timedOut ? "Timed out." : (signalMessage(r.signal, r.code) ?? undefinedBehaviour(r.stderr));
    if (crash) error = crash;
    if (r.timedOut) timedOut = true;
    const passed = !crash && normalizeOut(r.stdout) === normalizeOut(tc.expected);
    results.push({
      name: tc.name,
      passed,
      message: passed ? undefined : crash ?? `Expected output:\n${tc.expected}\n\nYour output:\n${r.stdout || "(nothing)"}`,
      ...(!passed && !crash ? { expected: tc.expected, actual: r.stdout } : {}),
    });
  }
  return { compiled: true, compileOutput, results, allPassed: results.every((r) => r.passed), output, error, ...(timedOut ? { timedOut } : {}) };
}

async function testCHarness(dir: string, ex: Exercise, code: string, nonce: string): Promise<TestResult> {
  const guard = await guardSharedSource("c", code, ex.starter);
  if (!guard.ok) return guard.result;
  const built = await compileToAssembly("c", cTestSource(code, ex.harness ?? "", guard.undefs));
  if (!built.ok && built.reach) return reachResult();
  return runCHarness(dir, nonce, async () => (built.ok ? compileCTest(dir, built) : built.step));
}

function testAsmHarness(dir: string, ex: Exercise, code: string, nonce: string): Promise<TestResult> {
  return runCHarness(dir, nonce, () => compile(dir, "asm", code, { test: true, harness: ex.harness ?? "" }));
}

/**
 * Results are only trusted if they carry this run's nonce. A line that merely looks like a
 * result is the learner's own output. For C and assembly the nonce reaches the harness through
 * the environment and is wiped before learner code runs (see cTestPrelude); the kernel harness
 * still compiles it in. Either way this stops copy-pasted or hard-coded forgeries, and testExercise
 * additionally requires the full expected transcript (docs/security.md, L1).
 */
function parseHarnessOutput(text: string, nonce: string) {
  const tag = (kind: string) => `@@${kind} ${nonce} `;
  const results: TestCaseResult[] = [];
  let running: string | null = null;
  let valid = true;
  const userOutput: string[] = [];
  for (const line of text.split("\n")) {
    if (line.startsWith(tag("RUN"))) {
      if (running !== null) valid = false;
      running = line.slice(tag("RUN").length);
    }
    else if (line.startsWith(tag("PASS"))) {
      const name = line.slice(tag("PASS").length);
      if (running !== name) valid = false;
      results.push({ name, passed: true });
      running = null;
    } else if (line.startsWith(tag("FAIL"))) {
      const [name, ...msg] = line.slice(tag("FAIL").length).split("\t");
      if (running !== name) valid = false;
      results.push({ name, passed: false, message: msg.join("\t") });
      running = null;
    } else if (line.trim() && !line.startsWith("@@EXC ")) userOutput.push(line);
  }
  return { results, running, userOutput, valid };
}

async function testKernelHarness(dir: string, ex: Exercise, code: string, nonce: string): Promise<TestResult> {
  const guard = await guardSharedSource("kernel", code, ex.starter);
  if (!guard.ok) return guard.result;
  const built = await compileToAssembly("kernel", kernelTestSource(code, ex.harness ?? "", nonce, guard.undefs));
  if (!built.ok && built.reach) return reachResult();
  const c = built.ok ? await compileKernelTest(dir, built) : built.step;
  const compileOutput = redactHarnessDiagnostics(cleanCompilerOutput(c.stderr + c.stdout), "kernel", 0);
  if (c.code !== 0) return { compiled: false, compileOutput, results: [], allPassed: false, output: "", error: "Your code didn't compile." };
  const k = await bootKernel(dir);
  const { results, running, valid } = parseHarnessOutput(k.debug, nonce);
  const error = kernelCrash(k);
  if (running) results.push({ name: running, passed: false, message: error ?? "The kernel stopped during this test." });
  const allPassed = !error && valid && running === null && results.length > 0 && results.every((t) => t.passed);
  return { compiled: true, compileOutput, results, allPassed, output: k.stdout.trim(), error: error ?? (!valid ? TESTS_INCOMPLETE : null), ...(k.timedOut ? { timedOut: true } : {}) };
}

async function runCHarness(dir: string, nonce: string, build: () => ReturnType<typeof exec>): Promise<TestResult> {
  const c = await build();
  const compileOutput = redactHarnessDiagnostics(cleanCompilerOutput(c.stderr + c.stdout), "c", 0);
  if (c.code !== 0) return { compiled: false, compileOutput, results: [], allPassed: false, output: "", error: "Your code didn't compile." };
  await keepOnlyProgram(dir);
  const r = await exec(dir, ["./prog"], { run: true, timeoutMs: RUN_TIMEOUT_MS, limits: runLimits(), env: { ...runEnv, BM_NONCE: nonce } });
  const { results, running, userOutput, valid } = parseHarnessOutput(r.stdout, nonce);
  const crash = r.timedOut ? `Timed out after ${RUN_TIMEOUT_MS / 1000}s — is there a loop that never ends?` : signalMessage(r.signal, r.code);
  const error = crash ?? undefinedBehaviour(r.stderr) ?? (r.code !== 0 ? `The program exited with status ${r.code}.` : null);
  if (running) results.push({ name: running, passed: false, message: error ?? "The program stopped during this test." });
  const allPassed = !error && valid && running === null && results.length > 0 && results.every((t) => t.passed);
  return { compiled: true, compileOutput, results, allPassed, output: [...userOutput, r.stderr.trim()].filter(Boolean).join("\n"), error: error ?? (!valid ? TESTS_INCOMPLETE : null), ...(r.timedOut ? { timedOut: true } : {}) };
}

// ---------- Out-of-process verdict (C lessons with a driver.c) ----------

// The learner's file is compiled alone, in a directory holding nothing else, to an object; the
// driver (VERDICT_PRELUDE + driver.c) is its own translation unit, built once per lesson together
// with the reference program and cached. Learner and reference run in separate sandboxes with the
// same seed, and the server compares their transcripts (src/lib/verdict.ts, docs/security.md L1).
// The reference build is ownerless (not charged to the learner's compile budget, not cancelled
// by one learner's disconnect) and a failed build is never cached, so the lesson fails closed.
type VerdictReference = { ok: true; driverObject: Buffer; program: Buffer } | { ok: false; reason: string };
const referenceCache = new Map<string, Promise<VerdictReference>>();
const MAX_REFERENCE_CACHE = 128;
const DRIVER_FLAGS = ["-std=c17", "-Wall", "-Wextra", "-O1", "-fno-builtin", "-fstack-protector-strong"];
const driverSource = (driver: string) => `${VERDICT_PRELUDE}\n#line 1 "driver.c"\n${driver}\n`;

function verdictReference(ex: Exercise): Promise<VerdictReference> {
  const key = createHash("sha256").update(VERDICT_PRELUDE).update("\0").update(ex.driver ?? "").update("\0").update(ex.solution).digest("hex");
  let hit = referenceCache.get(key);
  if (hit) {
    referenceCache.delete(key);
    referenceCache.set(key, hit);
    return hit;
  }
  const build = requestContext.exit(() => withWorkdir(async (dir): Promise<VerdictReference> => {
    await writeFile(path.join(dir, "driver.c"), driverSource(ex.driver ?? ""));
    const d = await exec(dir, ["gcc", ...DRIVER_FLAGS, "-c", "driver.c", "-o", "driver.o"], { timeoutMs: COMPILE_TIMEOUT_MS, limits: compileLimits(), artifact: "driver.o" });
    if (d.code !== 0) return { ok: false, reason: `driver.c does not compile:\n${cleanCompilerOutput(d.stderr + d.stdout)}` };
    await writeFile(path.join(dir, "main.c"), ex.solution);
    const p = await exec(dir, ["sh", "-c", `gcc ${C_FLAGS.join(" ")} -Dmain=__bm_student_main -c main.c -o main.o && gcc -fsanitize=undefined main.o driver.o -o prog -lm`], { timeoutMs: COMPILE_TIMEOUT_MS, limits: compileLimits(), artifact: "prog" });
    if (p.code !== 0) return { ok: false, reason: `the reference solution does not build with the driver:\n${cleanCompilerOutput(p.stderr + p.stdout)}` };
    return { ok: true, driverObject: await readFile(path.join(dir, "driver.o")), program: await readFile(path.join(dir, "prog")) };
  })).catch((error: unknown): VerdictReference => ({ ok: false, reason: String(error) }));
  hit = build.then((r) => {
    if (!r.ok && referenceCache.get(key) === hit) referenceCache.delete(key);
    return r;
  });
  referenceCache.set(key, hit);
  while (referenceCache.size > MAX_REFERENCE_CACHE) {
    const oldest = referenceCache.keys().next().value;
    if (oldest === undefined) break;
    referenceCache.delete(oldest);
  }
  return hit;
}

/** Run a verdict program (learner or reference) alone in a read-only work dir with this attempt's seed. */
async function runVerdictProgram(dir: string, seed: Buffer) {
  return exec(dir, ["./prog"], { run: true, timeoutMs: RUN_TIMEOUT_MS, limits: runLimits(), env: runEnv, seed });
}

/** The reference transcript for one seed, or why there is none. Exported for scripts/verify-curriculum.mts. */
export async function referenceTranscript(ex: Exercise, seed: Buffer): Promise<{ ok: true; transcript: Transcript; text: string } | { ok: false; reason: string }> {
  const ref = await verdictReference(ex);
  if (!ref.ok) return ref;
  return withWorkdir(async (dir) => {
    await writeFile(path.join(dir, "prog"), ref.program, { mode: 0o700 });
    const r = await runVerdictProgram(dir, seed);
    const text = r.records ?? "";
    const transcript = parseTranscript(text);
    if (r.timedOut || r.truncated || r.signal || r.code !== 0 || !transcript.valid || !transcript.complete || transcript.cases.length === 0) {
      return { ok: false, reason: `the reference run did not finish cleanly (exit ${r.code}${r.signal ? `, ${r.signal}` : ""}${r.timedOut ? ", timed out" : ""}): ${cleanCompilerOutput(r.stderr).slice(0, 500)}` };
    }
    return { ok: true, transcript, text };
  });
}

const REFERENCE_UNAVAILABLE = "This lesson's checks could not be prepared right now, so your code wasn't judged. Try again in a moment.";

async function testVerdict(dir: string, ex: Exercise, code: string, seed: Buffer): Promise<TestResult> {
  const fail = (compiled: boolean, compileOutput: string, error: string): TestResult => ({ compiled, compileOutput, results: [], allPassed: false, output: "", error });
  // 1. The learner's object, built where nothing but main.c exists (no driver, no solution).
  await writeFile(path.join(dir, "main.c"), code);
  const c = await exec(dir, ["gcc", ...C_FLAGS, "-Dmain=__bm_student_main", "-c", "main.c", "-o", "main.o"], { timeoutMs: COMPILE_TIMEOUT_MS, limits: compileLimits(), artifact: "main.o" });
  let compileOutput = cleanCompilerOutput(c.stderr + c.stdout);
  if (c.code !== 0) return fail(false, compileOutput, c.timedOut ? "Compilation took too long." : "Your code didn't compile.");
  // 2. The driver object and the reference program (cached per lesson content).
  const ref = await verdictReference(ex);
  if (!ref.ok) return fail(true, compileOutput, REFERENCE_UNAVAILABLE);
  // 3. Link. Only objects are present, so learner text can't read the driver source here either.
  await rm(path.join(dir, "main.c"), { force: true });
  await writeFile(path.join(dir, "driver.o"), ref.driverObject);
  const l = await exec(dir, ["gcc", "-fsanitize=undefined", "main.o", "driver.o", "-o", "prog", "-lm"], { timeoutMs: COMPILE_TIMEOUT_MS, limits: compileLimits(), artifact: "prog" });
  compileOutput = [compileOutput, cleanCompilerOutput(l.stderr + l.stdout).replace(/^.*\bin function `bm_driver_':\n?/gm, "")].filter(Boolean).join("\n");
  if (l.code !== 0) {
    const missing = [...compileOutput.matchAll(/undefined reference to `(\w+)'/g)].map((m) => m[1]);
    return fail(false, compileOutput, missing.length ? `The tests call ${[...new Set(missing)].join(", ")}, which your code doesn't define (check the name, and that it isn't static).` : "Your code didn't link.");
  }
  await keepOnlyProgram(dir);
  // 4. Learner run, then the reference run with the same seed in its own sandbox.
  const r = await runVerdictProgram(dir, seed);
  const want = await referenceTranscript(ex, seed);
  const output = [r.stdout.trim(), r.stderr.trim()].filter(Boolean).join("\n");
  if (!want.ok) return { ...fail(true, compileOutput, REFERENCE_UNAVAILABLE), output };
  const got = parseTranscript(r.records ?? "");
  const crash = r.timedOut
    ? `Timed out after ${RUN_TIMEOUT_MS / 1000}s — is there a loop that never ends?`
    : r.truncated
      ? "Output was cut off — your program printed a lot (maybe an infinite loop?)."
      : (signalMessage(r.signal, r.code) ?? undefinedBehaviour(r.stderr) ?? (r.code !== 0 ? `The program exited with status ${r.code}.` : null));
  const { results, intact } = compareTranscripts(want.transcript, got, crash);
  const allPassed = !crash && intact && results.length > 0 && results.every((t) => t.passed);
  return { compiled: true, compileOutput, results, allPassed, output, error: crash ?? (!intact ? TESTS_INCOMPLETE : null), ...(r.timedOut ? { timedOut: true } : {}) };
}

async function testGoHarness(dir: string, ex: Exercise, code: string): Promise<TestResult> {
  const c = await compile(dir, "go", code, { test: true, harness: ex.harness ?? "" });
  const compileOutput = redactHarnessDiagnostics(cleanCompilerOutput(c.stderr + c.stdout), "go", 0);
  if (c.code !== 0) return { compiled: false, compileOutput, results: [], allPassed: false, output: "", error: c.timedOut ? "Compilation took too long." : "Your code didn't compile.", ...(c.timedOut ? { timedOut: true } : {}) };
  await keepOnlyProgram(dir);
  const r = await exec(dir, ["./prog", "-test.v", "-test.timeout=5s", "-test.count=1"], { run: true, timeoutMs: RUN_TIMEOUT_MS * 2, limits: goRunLimits(), env: runEnv });
  const results: TestCaseResult[] = [];
  const lines = r.stdout.split("\n");
  // `go test -v` prints "=== RUN Name", the test's own output / t.Errorf lines (indented), then "--- PASS|FAIL: Name (0.00s)".
  let buf: string[] = [];
  for (const line of lines) {
    const m = line.match(/^\s*--- (PASS|FAIL): (\w+)/);
    if (m) {
      const msg = buf
        .map((l) => l.replace(/^\s+(\S+\.go:\d+: )?/, ""))
        .filter(Boolean)
        .join("\n");
      results.push({ name: m[2].replace(/^Test_?/, "").replaceAll("_", " "), passed: m[1] === "PASS", message: m[1] === "PASS" ? undefined : msg || "Test failed." });
      buf = [];
    } else if (line.startsWith("=== ")) {
      buf = [];
    } else if (!/^(PASS|FAIL|ok\s|exit status)/.test(line)) buf.push(line);
  }
  const timedOutInGo = /panic: test timed out/.test(r.stdout + r.stderr);
  const panic = (r.stdout + r.stderr).match(/^panic: .*$/m)?.[0];
  const crash = r.timedOut || timedOutInGo
    ? `Timed out after ${RUN_TIMEOUT_MS / 1000}s — is there a loop that never ends, or a goroutine waiting forever?`
    : /fatal error: all goroutines are asleep - deadlock!/.test(r.stdout + r.stderr)
      ? "fatal error: all goroutines are asleep - deadlock!"
      : panic ?? (r.signal ? signalMessage(r.signal, r.code) : null);
  const failedNames = new Set(results.map((t) => t.name));
  const running = [...r.stdout.matchAll(/^=== RUN\s+(\w+)/gm)].map((m) => m[1].replace(/^Test_?/, "").replaceAll("_", " ")).find((n) => !failedNames.has(n));
  if (running && crash) results.push({ name: running, passed: false, message: crash });
  const userOutput = r.stderr.trim();
  const allPassed = !crash && r.code === 0 && results.length > 0 && results.every((t) => t.passed);
  return { compiled: true, compileOutput, results, allPassed, output: userOutput, error: crash, ...(r.timedOut || timedOutInGo ? { timedOut: true } : {}) };
}

async function testRustHarness(dir: string, ex: Exercise, code: string): Promise<TestResult> {
  const c = await compile(dir, "rust", rustTestSource(code, ex.harness ?? ""), { test: true });
  const compileOutput = redactHarnessDiagnostics(cleanCompilerOutput(c.stderr + c.stdout), "rust", code.split("\n").length);
  if (c.code !== 0) return { compiled: false, compileOutput, results: [], allPassed: false, output: "", error: "Your code didn't compile." };
  await keepOnlyProgram(dir);
  const r = await exec(dir, ["./prog", "--test-threads=1", "--color", "never"], { run: true, timeoutMs: RUN_TIMEOUT_MS * 2, limits: runLimits(), env: runEnv });
  const results: TestCaseResult[] = [];
  const failures = new Map<string, string>();
  // Collect panic messages from the "---- name stdout ----" sections.
  const sections = r.stdout.split(/^---- /m).slice(1);
  for (const sec of sections) {
    const m = sec.match(/^__bm_tests::(\w+) stdout ----\n([\s\S]*?)(?=\n\n|\nfailures:|$)/);
    if (m) {
      const body = m[2]
        .split("\n")
        .filter((l) => !l.startsWith("note: run with") && !l.startsWith("thread '"))
        .join("\n")
        .trim();
      failures.set(m[1], body);
    }
  }
  for (const m of r.stdout.matchAll(/^test __bm_tests::(\w+) \.\.\. (ok|FAILED)$/gm)) {
    const name = m[1].replaceAll("_", " ");
    results.push({ name, passed: m[2] === "ok", message: m[2] === "ok" ? undefined : failures.get(m[1]) || "Test failed." });
  }
  const crash = r.timedOut ? "Timed out — is there a loop that never ends, or a connection that is never closed?" : r.signal ? signalMessage(r.signal, r.code) : null;
  const allPassed = !crash && r.code === 0 && results.length > 0 && results.every((t) => t.passed);
  return { compiled: true, compileOutput, results, allPassed, output: "", error: crash, ...(r.timedOut ? { timedOut: true } : {}) };
}

export async function cleanupStaleWorkdirs() {
  const entries = await readdir(tmpdir()).catch(() => [] as string[]);
  const cutoff = Date.now() - 15 * 60_000;
  await Promise.all(entries.filter((e) => e.startsWith("bm-run-")).map(async (e) => {
    const dir = path.join(tmpdir(), e);
    try {
      if (statSync(dir).mtimeMs < cutoff) await rm(dir, { recursive: true, force: true });
    } catch { /* Another server may already have cleaned this directory. */ }
  }));
}
