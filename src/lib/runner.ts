// Compiles and runs learner code inside a bubblewrap sandbox: no network,
// read-only /usr, a private /tmp and work dir, and CPU/memory/process/output
// limits. Imported only from route handlers and scripts (no "server-only" so
// scripts/verify-curriculum.ts can use it directly).
import { spawn } from "node:child_process";
import { mkdtemp, rm, writeFile, readdir } from "node:fs/promises";
import { existsSync, readdirSync, statSync } from "node:fs";
import { tmpdir, homedir, userInfo } from "node:os";
import path from "node:path";
import type { Exercise, Language } from "./curriculum/types";

const MAX_OUTPUT = 64 * 1024;
const COMPILE_TIMEOUT_MS = 20_000;
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

async function compile(dir: string, language: Language, source: string, opts: { test?: boolean } = {}) {
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
        const noMain = /undefined reference to `main'|`main` function not found/.test(compileOutput);
        return {
          ok: false,
          phase: "compile",
          compileOutput,
          stdout: "",
          stderr: "",
          exitCode: c.code,
          error: noMain
            ? "There's no main() function to run. Add one to experiment, or press Test to check your solution."
            : c.timedOut
              ? "Compilation took too long."
              : "Your code didn't compile. Read the first error carefully — later errors are often caused by the first.",
          durationMs: c.ms,
        };
      }
      const r = await exec(dir, ["./prog"], { timeoutMs: RUN_TIMEOUT_MS, stdin, limits: runLimits(), env: runEnv });
      const error = r.timedOut
        ? `Your program ran for more than ${RUN_TIMEOUT_MS / 1000} seconds and was stopped. Is there a loop that never ends, or is it waiting for input?`
        : r.truncated
          ? "Output was cut off at 64 KB — your program printed a lot (maybe an infinite loop?)."
          : (signalMessage(r.signal, r.code) ?? undefinedBehaviour(r.stderr));
      return { ok: !error && r.code === 0, phase: "run", compileOutput, stdout: r.stdout, stderr: r.stderr, exitCode: r.code, error, durationMs: c.ms + r.ms };
    }),
  );
}

const normalizeOut = (s: string) => s.replace(/\r\n/g, "\n").replace(/[ \t]+$/gm, "").trimEnd();

/** Run an exercise's tests against learner code. */
export function testExercise(exercise: Exercise, code: string): Promise<TestResult> {
  return withSlot(() =>
    withWorkdir(async (dir) => {
      if (exercise.mode === "stdout") return testStdout(dir, exercise, code);
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
    const r = await exec(dir, ["./prog"], { timeoutMs: RUN_TIMEOUT_MS, stdin: tc.stdin ?? "", limits: runLimits(), env: runEnv });
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

async function testCHarness(dir: string, ex: Exercise, code: string): Promise<TestResult> {
  const c = await compile(dir, "c", cTestSource(code, ex.harness ?? ""), { test: true });
  const compileOutput = cleanCompilerOutput(c.stderr + c.stdout);
  if (c.code !== 0) return { compiled: false, compileOutput, results: [], allPassed: false, output: "", error: "Your code didn't compile." };
  const r = await exec(dir, ["./prog"], { timeoutMs: RUN_TIMEOUT_MS, limits: runLimits(), env: runEnv });
  const results: TestCaseResult[] = [];
  let running: string | null = null;
  const userOutput: string[] = [];
  for (const line of r.stdout.split("\n")) {
    if (line.startsWith("@@RUN ")) running = line.slice(6);
    else if (line.startsWith("@@PASS ")) {
      results.push({ name: line.slice(7), passed: true });
      running = null;
    } else if (line.startsWith("@@FAIL ")) {
      const [name, ...msg] = line.slice(7).split("\t");
      results.push({ name, passed: false, message: msg.join("\t") });
      running = null;
    } else if (line.trim()) userOutput.push(line);
  }
  const crash = r.timedOut ? `Timed out after ${RUN_TIMEOUT_MS / 1000}s — is there a loop that never ends?` : signalMessage(r.signal, r.code);
  const error = crash ?? undefinedBehaviour(r.stderr);
  if (running) results.push({ name: running, passed: false, message: error ?? "The program stopped during this test." });
  const allPassed = !error && results.length > 0 && results.every((t) => t.passed);
  return { compiled: true, compileOutput, results, allPassed, output: [...userOutput, r.stderr.trim()].filter(Boolean).join("\n"), error };
}

async function testRustHarness(dir: string, ex: Exercise, code: string): Promise<TestResult> {
  const c = await compile(dir, "rust", rustTestSource(code, ex.harness ?? ""), { test: true });
  const compileOutput = cleanCompilerOutput(c.stderr + c.stdout);
  if (c.code !== 0) return { compiled: false, compileOutput, results: [], allPassed: false, output: "", error: "Your code didn't compile." };
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
