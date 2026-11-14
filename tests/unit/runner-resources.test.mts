import test from "node:test";
import assert from "node:assert/strict";
import { cleanupStaleCgroups, runnerResources } from "../../src/lib/runner-resources";

test("both allocation and stale cleanup reject paths outside a delegated cgroup subtree", async () => {
  const previous = process.env.RUNNER_CGROUP_ROOT;
  try {
    for (const root of ["/tmp/jobs", "/sys/fs/cgroup", "/sys/fs/cgroup/jobs/../../../../tmp/jobs"]) {
      process.env.RUNNER_CGROUP_ROOT = root;
      await assert.rejects(runnerResources(false), /delegated cgroup-v2 subtree/);
      await assert.rejects(cleanupStaleCgroups(), /delegated cgroup-v2 subtree/);
    }
  } finally {
    if (previous === undefined) delete process.env.RUNNER_CGROUP_ROOT;
    else process.env.RUNNER_CGROUP_ROOT = previous;
  }
});
