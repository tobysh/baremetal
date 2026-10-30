import { createServer, type Server } from "node:http";
import { chmod, lstat, unlink } from "node:fs/promises";
import { randomUUID } from "node:crypto";
import { findExercise, getTracks } from "../src/lib/curriculum";
import { validateLearningPaths } from "../src/lib/learning";
import { cleanupStaleWorkdirs, runnerStatus, runProgram, testExercise, withRunnerRequest } from "../src/lib/runner-engine";
import { RunnerBusyError, RUNNER_REQUEST_BYTES, RUNNER_RESPONSE_BYTES, RUNNER_REQUEST_TIMEOUT_MS, runnerRequestSchema, runResultSchema, testResultSchema } from "../src/lib/runner-protocol";

export async function startRunner(socket: string): Promise<Server> {
  if (!socket.startsWith("/") || Buffer.byteLength(socket) > 100) throw new Error("Runner needs an absolute Unix socket path of at most 100 bytes.");
  for (const key of ["AUTH_SECRET", "MONGODB_URI", "ANTHROPIC_API_KEY", "SMTP_URL"]) {
    if (process.env[key]) throw new Error(`Runner must not receive ${key}. Use a separate service environment.`);
  }
  if (process.env.NODE_ENV === "production" && !process.env.RUNNER_CGROUP_ROOT) throw new Error("Production runner requires RUNNER_CGROUP_ROOT.");
  // Never unlink an arbitrary file or a socket belonging to another process.
  const existing = await lstat(socket).catch((error: NodeJS.ErrnoException) => { if (error.code !== "ENOENT") throw error; return null; });
  if (existing) throw new Error("Runner socket already exists; stop the owner or remove the verified stale socket.");
  validateLearningPaths(getTracks());
  await cleanupStaleWorkdirs();
  const active = new Set<AbortController>();
  const server = createServer(async (req, res) => {
    const id = randomUUID();
    const started = Date.now();
    const controller = new AbortController();
    active.add(controller);
    const timer = setTimeout(() => { controller.abort(); res.destroy(); }, RUNNER_REQUEST_TIMEOUT_MS);
    res.on("close", () => { if (!res.writableFinished) controller.abort(); });
    req.on("aborted", () => controller.abort());
    const reply = (status: number, body: unknown) => {
      if (res.destroyed) return;
      const json = JSON.stringify(body);
      if (Buffer.byteLength(json) > RUNNER_RESPONSE_BYTES) { res.writeHead(502).end(); return; }
      res.writeHead(status, { "Content-Type": "application/json" }).end(json);
    };
    try {
      if (req.method === "GET" && req.url === "/health") { reply(200, { version: 1, ready: true, ...runnerStatus() }); return; }
      if (req.method !== "POST" || req.url !== "/v1/execute") { reply(404, { error: "Unknown endpoint" }); return; }
      if (req.headers["content-type"] !== "application/json") { reply(415, { error: "JSON required" }); return; }
      const chunks: Buffer[] = [];
      let bytes = 0;
      for await (const chunk of req) {
        bytes += Buffer.byteLength(chunk);
        if (bytes > RUNNER_REQUEST_BYTES) { reply(413, { error: "Request too large" }); return; }
        chunks.push(Buffer.from(chunk));
      }
      let body: unknown;
      try { body = JSON.parse(Buffer.concat(chunks).toString("utf8")); } catch { reply(400, { error: "Invalid JSON" }); return; }
      const parsed = runnerRequestSchema.safeParse(body);
      if (!parsed.success) { reply(400, { error: "Invalid runner request" }); return; }
      const payload = parsed.data;
      const exercise = findExercise(payload.exerciseId)?.exercise;
      if (!exercise) { reply(404, { error: "Unknown exercise" }); return; }
      const result = await withRunnerRequest(payload.keys, controller.signal, async () => payload.operation === "run"
        ? runResultSchema.parse(await runProgram(exercise.language, payload.code, payload.stdin))
        : testResultSchema.parse(await testExercise(exercise, payload.code)));
      reply(200, result);
    } catch (error) {
      if (controller.signal.aborted) { res.destroy(); return; }
      const busy = error instanceof RunnerBusyError;
      reply(busy ? 503 : 500, { error: busy ? "Runner busy" : "Runner failure" });
      console.error(JSON.stringify({ event: busy ? "runner_busy" : "runner_failure", id }));
    } finally {
      clearTimeout(timer);
      active.delete(controller);
      console.log(JSON.stringify({ event: "runner_request", id, durationMs: Date.now() - started, status: res.statusCode, cancelled: controller.signal.aborted, ...runnerStatus() }));
    }
  });
  server.requestTimeout = 15_000; // Bound request upload time; execution has its own deadline.
  server.headersTimeout = 10_000;
  server.maxConnections = 64;
  process.umask(0o007);
  await new Promise<void>((resolve, reject) => {
    server.once("error", reject);
    server.listen(socket, () => resolve());
  });
  await chmod(socket, 0o660);
  server.on("close", () => { void unlink(socket).catch(() => {}); });
  const stop = () => { for (const controller of active) controller.abort(); server.close(); server.closeAllConnections(); };
  process.once("SIGTERM", stop);
  process.once("SIGINT", stop);
  server.once("close", () => { process.off("SIGTERM", stop); process.off("SIGINT", stop); });
  return server;
}

if (process.argv[1]?.endsWith("runner-service.mts")) {
  const socket = process.env.RUNNER_SOCKET;
  if (!socket) throw new Error("Set RUNNER_SOCKET to a private Unix socket path.");
  await startRunner(socket);
  console.log(JSON.stringify({ event: "runner_ready", version: 1 }));
}
