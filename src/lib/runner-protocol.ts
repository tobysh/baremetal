import { z } from "zod";

export class RunnerBusyError extends Error {}
export class RunnerUnavailableError extends Error {}
export const RUNNER_REQUEST_BYTES = 512 * 1024;
export const RUNNER_RESPONSE_BYTES = 8 * 1024 * 1024;
export const RUNNER_REQUEST_TIMEOUT_MS = 10 * 60_000;

export const runnerRequestSchema = z.object({
  version: z.literal(1),
  operation: z.enum(["run", "test"]),
  exerciseId: z.string().min(1).max(200),
  code: z.string().max(64 * 1024),
  stdin: z.string().max(16 * 1024).default(""),
  keys: z.array(z.string().regex(/^[a-f0-9]{64}$/)).min(1).max(2),
}).strict();
export type RunnerRequest = z.infer<typeof runnerRequestSchema>;
const text = z.string().max(RUNNER_RESPONSE_BYTES);
export const runResultSchema = z.object({
  ok: z.boolean(), phase: z.enum(["compile", "run"]), compileOutput: text,
  stdout: text, stderr: text, exitCode: z.number().int().nullable(), error: text.nullable(), durationMs: z.number().nonnegative(),
}).strict();
export const testResultSchema = z.object({
  compiled: z.boolean(), compileOutput: text,
  results: z.array(z.object({ name: text, passed: z.boolean(), message: text.optional(), expected: text.optional(), actual: text.optional() }).strict()).max(10_000),
  allPassed: z.boolean(), output: text, error: text.nullable(),
}).strict();
