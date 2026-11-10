import test from "node:test";
import assert from "node:assert/strict";
import { mkdtemp, rm } from "node:fs/promises";
import { tmpdir } from "node:os";
import path from "node:path";
import { cleanupStaleWorkdirs, runProgram } from "../../src/lib/runner-engine";

test("failed workdir creation rejects execution and scan failures are visible without private metadata", async () => {
  const root = await mkdtemp(path.join(tmpdir(), "bm-diagnostics-"));
  const previous = process.env.TMPDIR;
  const originalError = console.error;
  const logs: string[] = [];
  try {
    // An absent temp root fails before any compiler or learner process starts.
    process.env.TMPDIR = path.join(root, "private-path-sentinel", "missing");
    console.error = (line: string) => { logs.push(line); };
    await assert.rejects(runProgram("c", "private-source-sentinel"), { code: "ENOENT" });
    await cleanupStaleWorkdirs();
    assert.deepEqual(logs.map((line) => JSON.parse(line)), [
      { event: "runner_workdir_create_failed", errorCode: "ENOENT" },
      { event: "runner_workdir_scan_failed", errorCode: "ENOENT" },
    ]);
    assert(!logs.join("\n").includes("private-path-sentinel"));
    assert(!logs.join("\n").includes("private-source-sentinel"));
  } finally {
    console.error = originalError;
    if (previous === undefined) delete process.env.TMPDIR;
    else process.env.TMPDIR = previous;
    await rm(root, { recursive: true, force: true });
  }
});
