import { mkdir, writeFile, rmdir, readdir, stat } from "node:fs/promises";
import { randomBytes } from "node:crypto";
import path from "node:path";
import { setTimeout as delay } from "node:timers/promises";

/** Requires an empty cgroup-v2 subtree delegated to the runner service uid. */
export async function runnerResources(compiling: boolean): Promise<{ command: string; prefix: string[]; cleanup: () => Promise<void> }> {
  const root = process.env.RUNNER_CGROUP_ROOT;
  if (!root) return { command: "bwrap", prefix: [], cleanup: async () => {} };
  const resolved = path.resolve(root);
  if (!resolved.startsWith("/sys/fs/cgroup/") || resolved === "/sys/fs/cgroup") throw new Error("RUNNER_CGROUP_ROOT must be a delegated cgroup-v2 subtree.");
  const group = path.join(resolved, `job-${process.pid}-${randomBytes(8).toString("hex")}`);
  await mkdir(group);
  const cleanup = async () => {
    // Kill descendants even when the sandbox leader exited before them.
    let killError: unknown;
    try { await writeFile(path.join(group, "cgroup.kill"), "1"); }
    catch (error) { killError = error; }
    for (let attempt = 0; attempt < 20; attempt++) {
      try { await rmdir(group); return; }
      catch (error) {
        const code = (error as NodeJS.ErrnoException).code;
        if (code === "ENOENT") return;
        if ((code !== "EBUSY" && code !== "ENOTEMPTY") || attempt === 19) {
          console.error("[runner] cgroup cleanup failed", { group, killError, error });
          return;
        }
        await delay(25);
      }
    }
  };
  try {
    await Promise.all([
      writeFile(path.join(group, "memory.max"), compiling ? "2147483648" : "536870912"),
      writeFile(path.join(group, "memory.swap.max"), "0"),
      // An OOM kill takes the whole job, not one process, so a half-killed sandbox can't linger.
      writeFile(path.join(group, "memory.oom.group"), "1"),
      writeFile(path.join(group, "pids.max"), compiling ? "512" : "128"),
      writeFile(path.join(group, "cpu.max"), "200000 100000"),
      writeFile(path.join(group, "io.weight"), "default 100"),
    ]);
    return {
      command: "/usr/bin/sh",
      // Join before bwrap or any learner/compiler code starts. The path is a positional
      // argument, never interpolated into shell code. Failure prevents execution.
      prefix: ["-c", 'printf "%s" "$$" > "$1/cgroup.procs" || exit 125; shift; exec bwrap "$@"', "runner-cgroup", group],
      cleanup,
    };
  } catch (error) {
    await cleanup();
    throw error;
  }
}

export async function cleanupStaleCgroups() {
  const root = process.env.RUNNER_CGROUP_ROOT;
  if (!root) return;
  const resolved = path.resolve(root);
  const entries = await readdir(resolved).catch(() => [] as string[]);
  const cutoff = Date.now() - 15 * 60_000;
  await Promise.all(entries.filter((e) => e.startsWith("job-")).map(async (e) => {
    const group = path.join(resolved, e);
    try {
      const info = await stat(group);
      if (info.mtimeMs >= cutoff) return;
    } catch { return; }
    try { await writeFile(path.join(group, "cgroup.kill"), "1"); } catch { /* Already empty or gone. */ }
    for (let attempt = 0; attempt < 20; attempt++) {
      try { await rmdir(group); return; }
      catch (error) {
        const code = (error as NodeJS.ErrnoException).code;
        if (code === "ENOENT") return;
        if ((code !== "EBUSY" && code !== "ENOTEMPTY") || attempt === 19) {
          console.error("[runner] stale cgroup cleanup failed", { group, error });
          return;
        }
        await delay(25);
      }
    }
  }));
}
