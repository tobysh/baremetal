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

const MAX_OUTPUT = 64 * 1024;
const COMPILE_TIMEOUT_MS = 20_000;
// Go has no precompiled standard library in this toolchain, so the first build in a fresh
// sandbox compiles what it imports (about 3 s wall, 25 s CPU across cores). Give it its own budget.
const GO_COMPILE_TIMEOUT_MS = 60_000;
const RUN_TIMEOUT_MS = 5_000;
const MAX_CONCURRENT = 4;
// CI may install Go outside /usr (for example, actions/setup-go's tool cache).
// When set, mount that toolchain into the sandbox at /go.
const GO_TOOLCHAIN_DIR = process.env.GO_TOOLCHAIN_DIR;

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
  cachedNproc = { at: Date.now(), limit: count + 48 };
  return cachedNproc.limit;
}

let active = 0;
export { RunnerBusyError } from "./runner-protocol";
import { RunnerBusyError } from "./runner-protocol";
import type { RunResult, TestResult, TestCaseResult } from "./runner-types";
export type { RunResult, TestResult, TestCaseResult } from "./runner-types";
const MAX_QUEUED = 24;
const QUEUE_TIMEOUT_MS = 30_000;
const requestContext = new AsyncLocalStorage<{ signal: AbortSignal }>();
const inFlight = new Map<string, number>();

/** Count queued and running requests per account and network, independently. */
export async function withRunnerRequest<T>(keys: string[], signal: AbortSignal, fn: () => Promise<T>): Promise<T> {
  const unique = [...new Set(keys)];
  if (unique.some((key) => (inFlight.get(key) ?? 0) >= 2)) throw new RunnerBusyError("You already have two runs in progress.");
  signal.throwIfAborted();
  for (const key of unique) inFlight.set(key, (inFlight.get(key) ?? 0) + 1);
  try {
    return await requestContext.run({ signal }, fn);
  } finally {
    for (const key of unique) {
      const count = (inFlight.get(key) ?? 1) - 1;
      if (count) inFlight.set(key, count);
      else inFlight.delete(key);
    }
  }
}

type Waiter = { grant: () => void };
const queue: Waiter[] = [];
export const runnerStatus = () => ({ active, queued: queue.length });
async function withSlot<T>(fn: () => Promise<T>): Promise<T> {
  const signal = requestContext.getStore()?.signal;
  signal?.throwIfAborted();
  if (active < MAX_CONCURRENT) active++;
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
      const waiter: Waiter = { grant: () => { clear(); resolve(); } };
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
    const next = queue.shift();
    if (next) next.grant(); // Transfer the occupied slot directly; new arrivals cannot steal it.
    else active--;
  }
}

type Exec = { stdout: string; stderr: string; code: number | null; signal: string | null; timedOut: boolean; truncated: boolean; ms: number };

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

function sandboxArgs(work: string, extraEnv: Record<string, string> = {}, opts: { run?: boolean; tmpMb?: number; artifact?: string } = {}): string[] {
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
  for (const [k, v] of Object.entries(extraEnv)) args.push("--setenv", k, v);
  return args;
}

async function exec(work: string, cmd: string[], opts: { timeoutMs: number; stdin?: string; limits: string[]; env?: Record<string, string>; run?: boolean; tmpMb?: number; artifact?: string }): Promise<Exec> {
  const resources = await runnerResources(!!opts.artifact);
  try {
    return await new Promise<Exec>((resolve) => {
      const signal = requestContext.getStore()?.signal;
      signal?.throwIfAborted();
      const started = Date.now();
      const args = [...sandboxArgs(work, opts.env, { run: opts.run, tmpMb: opts.tmpMb ?? (opts.run ? 32 : undefined), artifact: opts.artifact }), "--", "/usr/bin/prlimit", ...opts.limits, "--",
        ...(opts.artifact ? ["sh", "-c", 'cp -a /input/. /work/ && "$@" && cat "$BM_ARTIFACT" >&4', "build", ...cmd] : cmd)];
      if (opts.artifact) {
        const at = args.indexOf("--clearenv") + 1;
        args.splice(at, 0, "--setenv", "BM_ARTIFACT", opts.artifact);
      }
      const child = spawn(resources.command, [...resources.prefix, ...args], { stdio: ["pipe", "pipe", "pipe", "pipe", ...(opts.artifact ? ["pipe" as const] : [])] });
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
        stderr += `Sandbox could not start: ${error.message}`;
      });
      child.on("close", async (code, childSignal) => {
        clearTimeout(timer);
        signal?.removeEventListener("abort", abort);
        if (opts.artifact && code === 0 && binarySize <= 64 * MB) {
          try { await writeFile(path.join(work, opts.artifact), Buffer.concat(binary), { mode: 0o700 }); }
          catch (error) { stderr += String(error); code = 1; }
        }
        resolve({ stdout, stderr, code, signal: childSignal, timedOut, truncated, ms: Date.now() - started });
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
    return output
      .split(/\n\s*\n/)
      .map((block) => {
        const at = block.match(/-->\s*main\.rs:(\d+)/);
        return at && Number(at[1]) > learnerLines ? HIDDEN_NOTE : block;
      })
      .join("\n\n");
  }
  const harnessFiles = new Set(language === "go" ? ["bm_test.go"] : ["tests.c"]);
  const out: string[] = [];
  let hidden = false;
  for (const line of output.split("\n")) {
    const head = line.match(/^(?:In file included from )?(?:\.\/)?([^\s:]+\.[a-z]+):(?:\d+|\s+In )/);
    if (head) hidden = harnessFiles.has(head[1]);
    if (!hidden) out.push(line);
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

function cTestSource(student: string, harness: string) {
  // Learners may keep their own main() for experimenting; rename it so the
  // test harness's main() is the entry point.
  // main is renamed with -Dmain=... on the command line rather than a #define
  // here, so main.c's line numbers and gcc's source excerpts stay exact.
  return `${student}\n#undef main\n#line 1 "tests.c"\n${cTestPrelude()}\n${harness}\n`;
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

function kernelTestSource(student: string, harness: string, nonce: string) {
  // Same trick as the C harness: the learner may keep a kmain() to experiment, so rename it.
  return `${student}\n#undef kmain\n#line 1 "tests.c"\n${kernelTestPrelude(nonce)}\n${harness}\n`;
}

async function compileKernel(dir: string, source: string, test: boolean) {
  for (const f of ["boot.S", "runtime.c", "link.ld"]) await writeFile(path.join(dir, f), await readFile(path.join(KERNEL_SUPPORT, f)));
  await writeFile(path.join(dir, "kernel.c"), source);
  const rename = test ? "-Dkmain=__bm_student_kmain " : "";
  const script = [
    `gcc ${KERNEL_CFLAGS} -c boot.S runtime.c`,
    `gcc ${KERNEL_CFLAGS} ${rename}-c kernel.c`,
    `ld -m elf_x86_64 -nostdlib -z max-page-size=4096 -T link.ld -o kernel64.elf boot.o runtime.o kernel.o`,
    `objcopy -O elf32-i386 kernel64.elf kernel.elf`,
  ].join(" && ");
  return exec(dir, ["sh", "-c", script], { timeoutMs: COMPILE_TIMEOUT_MS, limits: compileLimits(), artifact: "kernel.elf" });
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
  for (const f of ["tests.c", "bm_init.c", "bm_init.o", "tests.o"]) await rm(path.join(dir, f), { force: true });
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
  return exec(dir, cmd, { timeoutMs: GO_COMPILE_TIMEOUT_MS, limits: goCompileLimits(), env: GO_ENV, tmpMb: 1024, artifact: "prog" });
}

async function compile(dir: string, language: Language, source: string, opts: { test?: boolean; harness?: string } = {}) {
  if (language === "kernel") return compileKernel(dir, source, !!opts.test);
  if (language === "go") return compileGo(dir, source, opts.test ? (opts.harness ?? "") : null);
  if (language === "asm") return compileAsm(dir, source, opts.test ? (opts.harness ?? "") : null);
  if (language === "c") {
    await writeFile(path.join(dir, "main.c"), source);
    await writeFile(path.join(dir, "bm_init.c"), C_STDIO_INIT);
    const rename = opts.test ? ["-Dmain=__bm_student_main"] : [];
    return exec(dir, ["gcc", ...C_FLAGS, ...rename, "main.c", "bm_init.c", "-o", "prog", "-lm"], { timeoutMs: COMPILE_TIMEOUT_MS, limits: compileLimits(), artifact: "prog" });
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
  return withSlot(() =>
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
      return { ok: !error && r.code === 0, phase: "run", compileOutput, stdout: r.stdout, stderr: r.stderr, exitCode: r.code, error, durationMs: c.ms + r.ms };
    }),
  );
}

/**
 * Test binaries run with only themselves in the work dir, so learner code can't read the
 * hidden test sources at runtime. (It can still embed them at compile time; see docs/security.md L2.)
 */
async function keepOnlyProgram(dir: string) {
  for (const f of await readdir(dir)) if (f !== "prog") await rm(path.join(dir, f), { recursive: true, force: true });
}

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
  c: [/#\s*embed\b|__has_embed/],
  kernel: [/#\s*embed\b|__has_embed/],
};

function harnessTampering(code: string, language: Language): string | null {
  const patterns = [...TAMPER_PATTERNS, ...(TAMPER_PATTERNS_BY_LANGUAGE[language] ?? [])];
  if (!patterns.some((re) => re.test(code))) return null;
  return "Your code contains text that belongs to the test harness (its result lines, internal names, load-time hooks or a read of /proc). Tests can't be checked while it's there. Remove it and run the tests again.";
}

/**
 * Replace the value in the first declaration of each variable (`int a = 7, b = 8;`, `let n: i32 = 3;`,
 * `x := 3`). Null if a variable can't be found.
 */
function setInitialValues(code: string, set: Record<string, string>): string | null {
  let out = code;
  for (const [name, value] of Object.entries(set)) {
    const re = new RegExp(`(\\b${name}\\b[^=;\\n]*?:?=\\s*)("[^"\\n]*"|'[^'\\n]*'|[^;\\n,]+)`);
    if (!re.test(out)) return null;
    out = out.replace(re, (_, head) => `${head}${value}`);
  }
  return out;
}

const normalizeOut = (s: string) => s.replace(/\r\n/g, "\n").replace(/[ \t]+$/gm, "").trimEnd();

/** Run an exercise's tests against learner code. */
export function testExercise(exercise: Exercise, code: string): Promise<TestResult> {
  return withSlot(() =>
    withWorkdir(async (dir) => {
      if (exercise.mode === "stdout") return testStdout(dir, exercise, code);
      const tampering = harnessTampering(code, exercise.language);
      if (tampering) return { compiled: true, compileOutput: "", results: [], allPassed: false, output: "", error: tampering };
      const result = await runHarness(dir, exercise, code);
      if (!result.allPassed) return result;
      // Every reported result must match what the harness really emits, so one forged PASS
      // followed by exit(0) can't pass a lesson.
      const expected = await expectedTestNames(exercise);
      if (!expected || !sameNames(expected, result.results.map((r) => r.name))) {
        return { ...result, allPassed: false, error: TESTS_INCOMPLETE };
      }
      return result;
    }),
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
  for (const tc of ex.cases ?? []) {
    if (tc.set) {
      const changed = setInitialValues(code, tc.set);
      if (changed === null) {
        results.push({ name: tc.name, passed: false, message: `Keep the starter's variables (${Object.keys(tc.set).join(", ")}) and calculate from them, so the program works for other numbers too.` });
        continue;
      }
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
    const passed = !crash && normalizeOut(r.stdout) === normalizeOut(tc.expected);
    results.push({
      name: tc.name,
      passed,
      message: passed ? undefined : crash ?? `Expected output:\n${tc.expected}\n\nYour output:\n${r.stdout || "(nothing)"}`,
      ...(!passed && !crash ? { expected: tc.expected, actual: r.stdout } : {}),
    });
  }
  return { compiled: true, compileOutput, results, allPassed: results.every((r) => r.passed), output, error };
}

function testCHarness(dir: string, ex: Exercise, code: string, nonce: string): Promise<TestResult> {
  return runCHarness(dir, nonce, () => compile(dir, "c", cTestSource(code, ex.harness ?? ""), { test: true }));
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
  const c = await compile(dir, "kernel", kernelTestSource(code, ex.harness ?? "", nonce), { test: true });
  const compileOutput = redactHarnessDiagnostics(cleanCompilerOutput(c.stderr + c.stdout), "kernel", 0);
  if (c.code !== 0) return { compiled: false, compileOutput, results: [], allPassed: false, output: "", error: "Your code didn't compile." };
  const k = await bootKernel(dir);
  const { results, running, valid } = parseHarnessOutput(k.debug, nonce);
  const error = kernelCrash(k);
  if (running) results.push({ name: running, passed: false, message: error ?? "The kernel stopped during this test." });
  const allPassed = !error && valid && running === null && results.length > 0 && results.every((t) => t.passed);
  return { compiled: true, compileOutput, results, allPassed, output: k.stdout.trim(), error: error ?? (!valid ? TESTS_INCOMPLETE : null) };
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
  return { compiled: true, compileOutput, results, allPassed, output: [...userOutput, r.stderr.trim()].filter(Boolean).join("\n"), error: error ?? (!valid ? TESTS_INCOMPLETE : null) };
}

async function testGoHarness(dir: string, ex: Exercise, code: string): Promise<TestResult> {
  const c = await compile(dir, "go", code, { test: true, harness: ex.harness ?? "" });
  const compileOutput = redactHarnessDiagnostics(cleanCompilerOutput(c.stderr + c.stdout), "go", 0);
  if (c.code !== 0) return { compiled: false, compileOutput, results: [], allPassed: false, output: "", error: c.timedOut ? "Compilation took too long." : "Your code didn't compile." };
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
  return { compiled: true, compileOutput, results, allPassed, output: userOutput, error: crash };
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
  return { compiled: true, compileOutput, results, allPassed, output: "", error: crash };
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
