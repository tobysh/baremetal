import assert from "node:assert/strict";
import { spawn } from "node:child_process";
import { mkdtemp, rm, stat } from "node:fs/promises";
import { request } from "node:http";
import { tmpdir } from "node:os";
import path from "node:path";
import { setTimeout as delay } from "node:timers/promises";
import { getTracks } from "../../src/lib/curriculum";
import { executeRun, executeTest, assertRunnerConfiguration, RunnerUnavailableError } from "../../src/lib/runner-client";
import { RUNNER_REQUEST_BYTES } from "../../src/lib/runner-protocol";

const dir = await mkdtemp(path.join(tmpdir(), "bm-socket-check-"));
const socket = path.join(dir, "runner.sock");
const child = spawn(process.execPath, ["--import", "tsx", "scripts/runner-service.mts"], {
  env: { PATH: process.env.PATH, NODE_ENV: "test", RUNNER_SOCKET: socket, ...(process.env.RUST_TOOLCHAIN_DIR ? { RUST_TOOLCHAIN_DIR: process.env.RUST_TOOLCHAIN_DIR } : {}), ...(process.env.GO_TOOLCHAIN_DIR ? { GO_TOOLCHAIN_DIR: process.env.GO_TOOLCHAIN_DIR } : {}) },
  stdio: ["ignore", "pipe", "pipe"],
});
let logs = "";
child.stdout.on("data", (chunk) => { logs += chunk; });
child.stderr.on("data", (chunk) => { logs += chunk; });
const raw = (body?: unknown, signal?: AbortSignal, endpoint = "/v1/execute") => new Promise<{ status: number; body: Record<string, unknown> }>((resolve, reject) => {
  const req = request({ socketPath: socket, path: endpoint, method: body === undefined ? "GET" : "POST", signal, headers: { "Content-Type": "application/json" } }, (res) => {
    let data = "";
    res.on("data", (chunk) => { data += chunk; });
    res.on("end", () => { try { resolve({ status: res.statusCode!, body: JSON.parse(data) }); } catch (error) { reject(error); } });
    res.on("error", reject);
  });
  req.on("error", reject);
  req.end(body === undefined ? undefined : JSON.stringify(body));
});
const oldSocket = process.env.RUNNER_SOCKET;
process.env.RUNNER_SOCKET = socket;
process.env.AUTH_SECRET ||= "runner-test-secret-only-32-characters-long";
try {
  let ready = false;
  for (let i = 0; i < 200; i++) {
    if (child.exitCode !== null) throw new Error(`Runner exited: ${logs}`);
    ready = await raw(undefined, undefined, "/health").then((res) => res.status === 200).catch(() => false);
    if (ready) break;
    await delay(50);
  }
  assert(ready, logs);
  assert.equal((await stat(socket)).mode & 0o777, 0o660);
  const tracks = getTracks();
  const exercise = tracks.find((track) => track.slug === "first-steps")!.lessons[0].exercise;
  const payload = { version: 1, operation: "run", exerciseId: exercise.id, code: '#include <stdio.h>\nint main(void){puts("socket works");}', stdin: "", keys: ["a".repeat(64)] };
  assert.equal((await raw({ ...payload, version: 2 })).status, 400);
  assert.equal((await raw({ ...payload, harness: "forged" })).status, 400);
  assert.equal((await raw({ ...payload, keys: ["raw-ip"] })).status, 400);
  assert.equal((await raw({ ...payload, code: "a".repeat(65537) })).status, 400);
  assert.equal((await raw({ ...payload, exerciseId: "does-not-exist" })).status, 404);
  // This is below the upload cap but above the source limit, and must not execute.
  assert.equal((await raw({ ...payload, code: "a".repeat(RUNNER_REQUEST_BYTES - 1024) })).status, 400);
  const context = { keys: ["test-user", "test-ip"], signal: new AbortController().signal };
  const run = await executeRun(exercise, payload.code, "", context);
  assert(run.ok, JSON.stringify(run));
  assert.equal(run.stdout.trim(), "socket works");
  for (const language of ["c", "rust", "go", "asm", "kernel"]) {
    const ex = tracks.find((track) => track.language === language)!.lessons[0].exercise;
    const result = await executeTest(ex, ex.solution, context);
    assert(result.allPassed, `${language}: ${JSON.stringify(result)}`);
    console.log(`Socket reference solution passed: ${language}`);
  }
  const slow = { ...payload, code: '#include <unistd.h>\nint main(void){sleep(4);}' };
  const one = new AbortController(), two = new AbortController();
  const first = raw(slow, one.signal).catch(() => null);
  const second = raw(slow, two.signal).catch(() => null);
  for (let i = 0; i < 100; i++) {
    const health = await raw(undefined, undefined, "/health");
    if (health.body.active === 2) break;
    await delay(20);
  }
  assert.equal((await raw(slow)).status, 503, "admission limits span requests in the service");
  one.abort(); two.abort();
  await Promise.all([first, second]);
  let drained = false;
  for (let i = 0; i < 150; i++) {
    const health = await raw(undefined, undefined, "/health");
    drained = health.body.active === 0 && health.body.queued === 0;
    if (drained) break;
    await delay(20);
  }
  assert(drained, "disconnect must release every active slot");
  const controllers = Array.from({ length: 32 }, () => new AbortController());
  const burst = controllers.map((controller, index) => raw({ ...slow, keys: [(index + 100).toString(16).padStart(64, "0")] }, controller.signal).catch(() => null));
  let saturated = false;
  for (let i = 0; i < 100; i++) {
    const health = await raw(undefined, undefined, "/health");
    // A pool is saturated when it runs all of its slots and holds a full queue (compile slots by default).
    type PoolStatus = { active: number; queued: number; slots: number };
    const status = health.body as unknown as { compile: PoolStatus; run: PoolStatus };
    const pool = status.compile.queued >= status.run.queued ? status.compile : status.run;
    saturated = pool.active === pool.slots && pool.queued === 24;
    if (saturated) break;
    await delay(10);
  }
  assert(saturated, "service must keep exactly its slot count of active jobs per pool and at most 24 waiting");
  // Give admission rejections time to reach their clients before cancelling the accepted work.
  await delay(50);
  for (const controller of controllers) controller.abort();
  const burstResults = await Promise.all(burst);
  assert(burstResults.some((result) => result?.status === 503), "overflow must return busy");
  drained = false;
  for (let i = 0; i < 150; i++) {
    const health = await raw(undefined, undefined, "/health");
    drained = health.body.active === 0 && health.body.queued === 0;
    if (drained) break;
    await delay(20);
  }
  assert(drained, "queued cancellations must remove all waiters and active work");
  const after = await raw(payload);
  assert.equal(after.status, 200);
  assert.equal(after.body.ok, true, "service remains usable after cancellation");
  const badSocket = socket + ".missing";
  process.env.RUNNER_SOCKET = badSocket;
  await assert.rejects(executeRun(exercise, payload.code, "", context), RunnerUnavailableError);
  const originalNodeEnv = process.env.NODE_ENV;
  Object.assign(process.env, { NODE_ENV: "production" });
  delete process.env.RUNNER_SOCKET;
  assert.throws(assertRunnerConfiguration, /requires RUNNER_SOCKET/);
  if (originalNodeEnv === undefined) Reflect.deleteProperty(process.env, "NODE_ENV"); else Object.assign(process.env, { NODE_ENV: originalNodeEnv });
  assert(!logs.includes("test-user") && !logs.includes("test-ip") && !logs.includes(payload.code), "service logs contain no source or identities");
  console.log("Runner socket validation, all languages, admission, cancellation and fail-closed checks passed.");
} finally {
  if (oldSocket === undefined) delete process.env.RUNNER_SOCKET; else process.env.RUNNER_SOCKET = oldSocket;
  child.kill("SIGTERM");
  await new Promise<void>((resolve) => { if (child.exitCode !== null) resolve(); else child.once("exit", () => resolve()); });
  await rm(dir, { recursive: true, force: true });
}
