import { request } from "node:http";
import { randomUUID } from "node:crypto";
import { hmac } from "./crypto";
import type { Exercise } from "./curriculum/types";
import type { RunResult, TestResult } from "./runner-types";
import { RunnerBusyError, RunnerUnavailableError, RUNNER_REQUEST_BYTES, RUNNER_RESPONSE_BYTES, RUNNER_REQUEST_TIMEOUT_MS, runnerRequestSchema, runResultSchema, testResultSchema, type RunnerRequest } from "./runner-protocol";

export { RunnerBusyError, RunnerUnavailableError } from "./runner-protocol";
type Context = { keys: string[]; signal: AbortSignal };
const hashedKeys = (context: Context) => context.keys.map((key) => Buffer.from(hmac("runner-admission", key), "base64url").toString("hex"));
const keyKinds = (context: Context) => context.keys.map((key) => (key.startsWith("ip:") ? "ip" : "user") as "user" | "ip");

export function assertRunnerConfiguration() {
  const socket = process.env.RUNNER_SOCKET;
  if (socket && (!socket.startsWith("/") || Buffer.byteLength(socket) > 100)) throw new Error("RUNNER_SOCKET must be an absolute Unix socket path of at most 100 bytes.");
  if (process.env.NODE_ENV === "production" && !socket) throw new Error("Production requires RUNNER_SOCKET; local execution under the web uid is disabled.");
}

async function callRunner(payload: RunnerRequest, signal: AbortSignal): Promise<unknown> {
  signal.throwIfAborted();
  const serialized = JSON.stringify(runnerRequestSchema.parse(payload));
  if (Buffer.byteLength(serialized) > RUNNER_REQUEST_BYTES) throw new RunnerUnavailableError("Runner request exceeds transport limit.");
  return new Promise((resolve, reject) => {
    const req = request({ socketPath: process.env.RUNNER_SOCKET!, path: "/v1/execute", method: "POST", signal,
      headers: { "Content-Type": "application/json", "Content-Length": Buffer.byteLength(serialized), "X-Request-Id": randomUUID() } }, (res) => {
      const chunks: Buffer[] = [];
      let bytes = 0;
      res.on("data", (chunk: Buffer) => {
        bytes += chunk.length;
        if (bytes > RUNNER_RESPONSE_BYTES) { res.destroy(new RunnerUnavailableError("Runner response exceeded its limit.")); return; }
        chunks.push(chunk);
      });
      res.on("error", () => reject(new RunnerUnavailableError("Runner response interrupted.")));
      res.on("end", () => {
        if (res.statusCode === 503) return reject(new RunnerBusyError("The code runner is busy."));
        if (res.statusCode !== 200) return reject(new RunnerUnavailableError("The code runner could not complete the request."));
        try {
          const body: unknown = JSON.parse(Buffer.concat(chunks).toString("utf8"));
          const parsed = payload.operation === "run" ? runResultSchema.parse(body) : testResultSchema.parse(body);
          resolve(parsed);
        } catch { reject(new RunnerUnavailableError("Invalid runner response.")); }
      });
    });
    const timer = setTimeout(() => req.destroy(new RunnerUnavailableError("Runner request timed out.")), RUNNER_REQUEST_TIMEOUT_MS);
    req.on("close", () => clearTimeout(timer));
    req.on("error", (error) => reject(signal.aborted ? error : new RunnerUnavailableError("The code runner is unavailable.")));
    req.end(serialized);
  });
}

export async function executeRun(exercise: Exercise, code: string, stdin: string, context: Context): Promise<RunResult> {
  assertRunnerConfiguration();
  if (!process.env.RUNNER_SOCKET) {
    const engine = await import("./runner-engine");
    return engine.withRunnerRequest(context.keys, context.signal, () => engine.runProgram(exercise.language, code, stdin), keyKinds(context));
  }
  return await callRunner({ version: 1, operation: "run", exerciseId: exercise.id, code, stdin, keys: hashedKeys(context), kinds: keyKinds(context) }, context.signal) as RunResult;
}

export async function executeTest(exercise: Exercise, code: string, context: Context): Promise<TestResult> {
  assertRunnerConfiguration();
  if (!process.env.RUNNER_SOCKET) {
    const engine = await import("./runner-engine");
    return engine.withRunnerRequest(context.keys, context.signal, () => engine.testExercise(exercise, code), keyKinds(context));
  }
  return await callRunner({ version: 1, operation: "test", exerciseId: exercise.id, code, stdin: "", keys: hashedKeys(context), kinds: keyKinds(context) }, context.signal) as TestResult;
}
