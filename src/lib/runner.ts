// Compiles and runs learner code inside a bubblewrap sandbox: no network,
// read-only /usr, a private /tmp and work dir, and CPU/memory/process/output
// limits. Imported only from route handlers and scripts (no "server-only" so
// scripts/verify-curriculum.ts can use it directly).
import { spawn } from "node:child_process";
import { mkdtemp, rm, writeFile, readdir, readFile } from "node:fs/promises";
import { existsSync, readdirSync, statSync } from "node:fs";
import { tmpdir, homedir, userInfo } from "node:os";
import path from "node:path";
import type { Exercise, Language } from "./curriculum/types";

const MAX_OUTPUT = 64 * 1024;
const COMPILE_TIMEOUT_MS = 20_000;
// Go has no precompiled standard library in this toolchain, so the first build in a fresh
// sandbox compiles what it imports (about 3 s wall, 25 s CPU across cores). Give it its own budget.
const GO_COMPILE_TIMEOUT_MS = 60_000;
const RUN_TIMEOUT_MS = 5_000;
const MAX_CONCURRENT = 4;

function findRustToolchain(): string | null {
  if (process.env.RUST_TOOLCHAIN_DIR) return process.env.RUST_TOOLCHAIN_DIR;
  const base = path.join(homedir(), ".rustup", "toolchains");
  if (!existsSync(base)) return null;
  const stable = readdirSync(base).find((d) => d.startsWith("stable-"));
  return stable ? path.join(base, stable) : null;
}
const RUST_TC = findRustToolchain();

// RLIMIT_NPROC counts every process owned by the real uid, so the limit is
// "what this user already runs + headroom" to stop fork bombs without
// breaking the rest of the desktop.
function nprocLimit(): number {
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
  return count + 48;
}

let active = 0;
const queue: (() => void)[] = [];
async function withSlot<T>(fn: () => Promise<T>): Promise<T> {
  if (active >= MAX_CONCURRENT) await new Promise<void>((r) => queue.push(r));
  active++;
  try {
    return await fn();
  } finally {
    active--;
    queue.shift()?.();
  }
}

type Exec = { stdout: string; stderr: string; code: number | null; signal: string | null; timedOut: boolean; truncated: boolean; ms: number };

function sandboxArgs(work: string, extraEnv: Record<string, string> = {}): string[] {
  const args = [
    "--unshare-all",
    "--die-with-parent",
    "--new-session",
    "--ro-bind", "/usr", "/usr",
    "--symlink", "usr/lib", "/lib",
    "--symlink", "usr/lib64", "/lib64",
    "--symlink", "usr/bin", "/bin",
    "--ro-bind", "/etc/ld.so.cache", "/etc/ld.so.cache",
    "--proc", "/proc",
    "--dev", "/dev",
    "--tmpfs", "/tmp",
    "--bind", work, "/work",
    "--chdir", "/work",
    "--clearenv",
    "--setenv", "PATH", "/usr/bin",
    "--setenv", "HOME", "/work",
    "--setenv", "LANG", "C.UTF-8",
  ];
  if (RUST_TC) args.push("--ro-bind", RUST_TC, RUST_TC);
  for (const [k, v] of Object.entries(extraEnv)) args.push("--setenv", k, v);
  return args;
}

function exec(work: string, cmd: string[], opts: { timeoutMs: number; stdin?: string; limits: string[]; env?: Record<string, string> }): Promise<Exec> {
  return new Promise((resolve) => {
    const started = Date.now();
    const args = [...sandboxArgs(work, opts.env), "--", "/usr/bin/prlimit", ...opts.limits, "--", ...cmd];
    const child = spawn("bwrap", args, { stdio: ["pipe", "pipe", "pipe"] });
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
    child.on("close", (code, signal) => {
      clearTimeout(timer);
      resolve({ stdout, stderr, code, signal, timedOut, truncated, ms: Date.now() - started });
    });
    child.stdin.on("error", () => {});
    child.stdin.end(opts.stdin ?? "");
  });
}

const MB = 1024 * 1024;
const compileLimits = () => [`--as=${2048 * MB}`, `--cpu=30`, `--nproc=${nprocLimit()}`, `--fsize=${64 * MB}`];
const goCompileLimits = () => [`--cpu=120`, `--nproc=${nprocLimit() + 200}`, `--fsize=${256 * MB}`];
// The Go runtime reserves a lot of address space up front, so the address-space cap is looser than for C.
const goRunLimits = () => [`--as=${4096 * MB}`, `--cpu=10`, `--nproc=${nprocLimit() + 64}`, `--fsize=${4 * MB}`, `--nofile=128`, `--core=0`];
const runLimits = () => [`--as=${256 * MB}`, `--cpu=5`, `--nproc=${nprocLimit()}`, `--fsize=${4 * MB}`, `--nofile=64`, `--core=0`];

// Strip the sandbox paths from compiler output so messages read naturally.
const cleanCompilerOutput = (s: string) => s.replaceAll("/work/", "").trim();

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

const C_TEST_PRELUDE = `
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#define CHECK(name, cond, ...) do { \\
  printf("\\n@@RUN %s\\n", name); fflush(stdout); \\
  if (cond) { printf("\\n@@PASS %s\\n", name); } \\
  else { printf("\\n@@FAIL %s\\t", name); printf(__VA_ARGS__); printf("\\n"); } \\
  fflush(stdout); } while (0)
`;

function cTestSource(student: string, harness: string) {
  // Learners may keep their own main() for experimenting; rename it so the
  // test harness's main() is the entry point.
  // main is renamed with -Dmain=... on the command line rather than a #define
  // here, so main.c's line numbers and gcc's source excerpts stay exact.
  return `${student}\n#undef main\n#line 1 "tests.c"\n${C_TEST_PRELUDE}\n${harness}\n`;
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

const KERNEL_TEST_PRELUDE = `
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
  bm_printf("\\n@@RUN %s\\n", name); \\
  if (cond) { bm_printf("\\n@@PASS %s\\n", name); } \\
  else { bm_printf("\\n@@FAIL %s\\t", name); bm_printf(__VA_ARGS__); bm_printf("\\n"); } } while (0)
`;

function kernelTestSource(student: string, harness: string) {
  // Same trick as the C harness: the learner may keep a kmain() to experiment, so rename it.
  return `${student}\n#undef kmain\n#line 1 "tests.c"\n${KERNEL_TEST_PRELUDE}\n${harness}\n`;
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
  return exec(dir, ["sh", "-c", script], { timeoutMs: COMPILE_TIMEOUT_MS, limits: compileLimits() });
}

// QEMU needs a lot of address space, so no --as limit here; CPU time, process count, file
// size and the wall-clock timeout still bound it. No network, no disk, no display.
const qemuLimits = () => [`--cpu=10`, `--nproc=${nprocLimit()}`, `--fsize=${4 * MB}`, `--nofile=256`, `--core=0`];

type KernelRun = Exec & { debug: string; exception: string | null; exitStatus: number | null };

async function bootKernel(dir: string): Promise<KernelRun> {
  const qemu = [
    "qemu-system-x86_64", "-machine", "pc,accel=tcg", "-cpu", "qemu64", "-m", "64",
    "-nodefaults", "-vga", "std", "-display", "none", "-no-reboot", "-monitor", "none", "-net", "none",
    "-kernel", "kernel.elf", "-serial", "stdio", "-debugcon", "file:debug.log",
    "-device", "isa-debug-exit,iobase=0xf4,iosize=0x04",
  ];
  const r = await exec(dir, qemu, { timeoutMs: RUN_TIMEOUT_MS, limits: qemuLimits() });
  const debug = (await readFile(path.join(dir, "debug.log"), "utf8").catch(() => "")).slice(0, MAX_OUTPUT);
  const exc = debug.match(/^@@EXC (.*)\trip=(0x[0-9a-f]+) err=(0x[0-9a-f]+) cr2=(0x[0-9a-f]+)$/m);
  const exception = exc ? `The CPU raised ${exc[1]} at instruction address ${exc[2]}${exc[1].includes("page fault") ? ` while touching address ${exc[4]}` : ""}.` : null;
  return { ...r, debug, exception, exitStatus: r.code };
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

export type RunResult = {
  ok: boolean;
  phase: "compile" | "run";
  compileOutput: string;
  stdout: string;
  stderr: string;
  exitCode: number | null;
  error: string | null;
  durationMs: number;
};

export type TestCaseResult = { name: string; passed: boolean; message?: string };
export type TestResult = {
  compiled: boolean;
  compileOutput: string;
  results: TestCaseResult[];
  allPassed: boolean;
  output: string;
  error: string | null;
};

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
  await writeFile(path.join(dir, "starter.s"), source);
  await writeFile(path.join(dir, "bm_init.c"), C_STDIO_INIT);
  const files = ["starter.s", "bm_init.c"];
  if (harness !== null) {
    await writeFile(path.join(dir, "tests.c"), `${C_TEST_PRELUDE}\n#line 1 "tests.c"\n${harness}\n`);
    files.push("tests.c");
  }
  return exec(dir, ["gcc", "-std=c17", "-Wall", "-Wextra", "-g", "-O0", "-no-pie", "-Wa,--noexecstack", ...files, "-o", "prog"], { timeoutMs: COMPILE_TIMEOUT_MS, limits: compileLimits() });
}

// Go: the learner's file is main.go (package main) in a throwaway module. Tests are a hidden
// bm_test.go in the same package, built with `go test -c` and run with -test.v. No network,
// no cgo; the build cache lives in the sandbox's private /tmp.
const GO_ENV = {
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
  const cmd = ["go"];
  if (harness !== null) {
    await writeFile(path.join(dir, "bm_test.go"), harness);
    cmd.push("test", "-c", "-vet=off", "-o", "prog");
  } else cmd.push("build", "-o", "prog", ".");
  return exec(dir, cmd, { timeoutMs: GO_COMPILE_TIMEOUT_MS, limits: goCompileLimits(), env: GO_ENV });
}

async function compile(dir: string, language: Language, source: string, opts: { test?: boolean; harness?: string } = {}) {
  if (language === "kernel") return compileKernel(dir, source, !!opts.test);
  if (language === "go") return compileGo(dir, source, opts.test ? (opts.harness ?? "") : null);
  if (language === "asm") return compileAsm(dir, source, opts.test ? (opts.harness ?? "") : null);
  if (language === "c") {
    await writeFile(path.join(dir, "main.c"), source);
    await writeFile(path.join(dir, "bm_init.c"), C_STDIO_INIT);
    const rename = opts.test ? ["-Dmain=__bm_student_main"] : [];
    return exec(dir, ["gcc", ...C_FLAGS, ...rename, "main.c", "bm_init.c", "-o", "prog", "-lm"], { timeoutMs: COMPILE_TIMEOUT_MS, limits: compileLimits() });
  }
  if (!RUST_TC) throw new Error("Rust toolchain not found. Install it with rustup or set RUST_TOOLCHAIN_DIR.");
  await writeFile(path.join(dir, "main.rs"), source);
  const args = [`${RUST_TC}/bin/rustc`, "--edition", "2021", "-C", "opt-level=0", "--color", "never", "main.rs", "-o", "prog"];
  if (opts.test) args.push("--test", "-A", "dead_code");
  return exec(dir, args, { timeoutMs: COMPILE_TIMEOUT_MS, limits: compileLimits() });
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
      const r = await exec(dir, ["./prog"], { timeoutMs: RUN_TIMEOUT_MS, stdin, limits: language === "go" ? goRunLimits() : runLimits(), env: runEnv });
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
 * hidden test sources at runtime. (It can still embed them at compile time; see docs/security-audit.md L2.)
 */
async function keepOnlyProgram(dir: string) {
  for (const f of await readdir(dir)) if (f !== "prog") await rm(path.join(dir, f), { recursive: true, force: true });
}

const normalizeOut = (s: string) => s.replace(/\r\n/g, "\n").replace(/[ \t]+$/gm, "").trimEnd();

/** Run an exercise's tests against learner code. */
export function testExercise(exercise: Exercise, code: string): Promise<TestResult> {
  return withSlot(() =>
    withWorkdir(async (dir) => {
      if (exercise.mode === "stdout") return testStdout(dir, exercise, code);
      if (exercise.language === "kernel") return testKernelHarness(dir, exercise, code);
      if (exercise.language === "go") return testGoHarness(dir, exercise, code);
      if (exercise.language === "asm") return testAsmHarness(dir, exercise, code);
      if (exercise.language === "c") return testCHarness(dir, exercise, code);
      return testRustHarness(dir, exercise, code);
    }),
  );
}

async function testStdout(dir: string, ex: Exercise, code: string): Promise<TestResult> {
  const c = await compile(dir, ex.language, code);
  const compileOutput = cleanCompilerOutput(c.stderr + c.stdout);
  if (c.code !== 0) return { compiled: false, compileOutput, results: [], allPassed: false, output: "", error: "Your code didn't compile." };
  const results: TestCaseResult[] = [];
  let output = "";
  let error: string | null = null;
  for (const tc of ex.cases ?? []) {
    const r = await exec(dir, ["./prog"], { timeoutMs: RUN_TIMEOUT_MS, stdin: tc.stdin ?? "", limits: ex.language === "go" ? goRunLimits() : runLimits(), env: runEnv });
    output = r.stdout + r.stderr;
    const crash = r.timedOut ? "Timed out." : (signalMessage(r.signal, r.code) ?? undefinedBehaviour(r.stderr));
    if (crash) error = crash;
    const passed = !crash && normalizeOut(r.stdout) === normalizeOut(tc.expected);
    results.push({
      name: tc.name,
      passed,
      message: passed ? undefined : crash ?? `Expected output:\n${tc.expected}\n\nYour output:\n${r.stdout || "(nothing)"}`,
    });
  }
  return { compiled: true, compileOutput, results, allPassed: results.every((r) => r.passed), output, error };
}

function testCHarness(dir: string, ex: Exercise, code: string): Promise<TestResult> {
  return runCHarness(dir, () => compile(dir, "c", cTestSource(code, ex.harness ?? ""), { test: true }));
}

function testAsmHarness(dir: string, ex: Exercise, code: string): Promise<TestResult> {
  return runCHarness(dir, () => compile(dir, "asm", code, { test: true, harness: ex.harness ?? "" }));
}

/** Reads the @@RUN / @@PASS / @@FAIL protocol the CHECK macro prints. Other lines are the learner's own output. */
function parseHarnessOutput(text: string) {
  const results: TestCaseResult[] = [];
  let running: string | null = null;
  const userOutput: string[] = [];
  for (const line of text.split("\n")) {
    if (line.startsWith("@@RUN ")) running = line.slice(6);
    else if (line.startsWith("@@PASS ")) {
      results.push({ name: line.slice(7), passed: true });
      running = null;
    } else if (line.startsWith("@@FAIL ")) {
      const [name, ...msg] = line.slice(7).split("\t");
      results.push({ name, passed: false, message: msg.join("\t") });
      running = null;
    } else if (line.trim() && !line.startsWith("@@EXC ")) userOutput.push(line);
  }
  return { results, running, userOutput };
}

async function testKernelHarness(dir: string, ex: Exercise, code: string): Promise<TestResult> {
  const c = await compile(dir, "kernel", kernelTestSource(code, ex.harness ?? ""), { test: true });
  const compileOutput = cleanCompilerOutput(c.stderr + c.stdout);
  if (c.code !== 0) return { compiled: false, compileOutput, results: [], allPassed: false, output: "", error: "Your code didn't compile." };
  const k = await bootKernel(dir);
  const { results, running } = parseHarnessOutput(k.debug);
  const error = kernelCrash(k);
  if (running) results.push({ name: running, passed: false, message: error ?? "The kernel stopped during this test." });
  const allPassed = !error && results.length > 0 && results.every((t) => t.passed);
  return { compiled: true, compileOutput, results, allPassed, output: k.stdout.trim(), error };
}

async function runCHarness(dir: string, build: () => ReturnType<typeof exec>): Promise<TestResult> {
  const c = await build();
  const compileOutput = cleanCompilerOutput(c.stderr + c.stdout);
  if (c.code !== 0) return { compiled: false, compileOutput, results: [], allPassed: false, output: "", error: "Your code didn't compile." };
  await keepOnlyProgram(dir);
  const r = await exec(dir, ["./prog"], { timeoutMs: RUN_TIMEOUT_MS, limits: runLimits(), env: runEnv });
  const { results, running, userOutput } = parseHarnessOutput(r.stdout);
  const crash = r.timedOut ? `Timed out after ${RUN_TIMEOUT_MS / 1000}s — is there a loop that never ends?` : signalMessage(r.signal, r.code);
  const error = crash ?? undefinedBehaviour(r.stderr);
  if (running) results.push({ name: running, passed: false, message: error ?? "The program stopped during this test." });
  const allPassed = !error && results.length > 0 && results.every((t) => t.passed);
  return { compiled: true, compileOutput, results, allPassed, output: [...userOutput, r.stderr.trim()].filter(Boolean).join("\n"), error };
}

async function testGoHarness(dir: string, ex: Exercise, code: string): Promise<TestResult> {
  const c = await compile(dir, "go", code, { test: true, harness: ex.harness ?? "" });
  const compileOutput = cleanCompilerOutput(c.stderr + c.stdout);
  if (c.code !== 0) return { compiled: false, compileOutput, results: [], allPassed: false, output: "", error: c.timedOut ? "Compilation took too long." : "Your code didn't compile." };
  await keepOnlyProgram(dir);
  const r = await exec(dir, ["./prog", "-test.v", "-test.timeout=5s", "-test.count=1"], { timeoutMs: RUN_TIMEOUT_MS * 2, limits: goRunLimits(), env: runEnv });
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
  const allPassed = !crash && results.length > 0 && results.every((t) => t.passed);
  return { compiled: true, compileOutput, results, allPassed, output: userOutput, error: crash };
}

async function testRustHarness(dir: string, ex: Exercise, code: string): Promise<TestResult> {
  const c = await compile(dir, "rust", rustTestSource(code, ex.harness ?? ""), { test: true });
  const compileOutput = cleanCompilerOutput(c.stderr + c.stdout);
  if (c.code !== 0) return { compiled: false, compileOutput, results: [], allPassed: false, output: "", error: "Your code didn't compile." };
  await keepOnlyProgram(dir);
  const r = await exec(dir, ["./prog", "--test-threads=1", "--color", "never"], { timeoutMs: RUN_TIMEOUT_MS * 2, limits: runLimits(), env: runEnv });
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
  const allPassed = !crash && results.length > 0 && results.every((t) => t.passed);
  return { compiled: true, compileOutput, results, allPassed, output: "", error: crash };
}

export async function cleanupStaleWorkdirs() {
  const entries = await readdir(tmpdir()).catch(() => [] as string[]);
  await Promise.all(entries.filter((e) => e.startsWith("bm-run-")).map((e) => rm(path.join(tmpdir(), e), { recursive: true, force: true })));
}
