// Run only on the disposable deployment validation host, as root.
import assert from "node:assert/strict";
import { execFile } from "node:child_process";
import { promisify } from "node:util";
import { readFile, readdir, readlink, stat } from "node:fs/promises";
import { request } from "node:http";
import { connect } from "node:net";
import { setTimeout as delay } from "node:timers/promises";
import path from "node:path";

if (!process.argv.includes("--disposable") || process.getuid?.() !== 0) throw new Error("Requires root and --disposable on a disposable deployment host.");
const secretFile = await stat("/etc/baremetal/web.env");
assert(secretFile.isFile() && secretFile.uid === 0 && (secretFile.mode & 0o777) === 0o600, "web environment must exist and be root-owned mode 0600 before checking isolation");
// Establish that the host database is reachable, so isolation cannot pass just
// because MongoDB was stopped before the probe.
await new Promise<void>((resolve, reject) => {
  const socket = connect(27017, "127.0.0.1");
  socket.setTimeout(2000, () => socket.destroy(new Error("Host MongoDB is not reachable.")));
  socket.once("error", reject);
  socket.once("connect", () => { socket.destroy(); resolve(); });
});
const exec = (command: string, args: string[]) => promisify(execFile)(command, args, { timeout: 5000, maxBuffer: 1024 * 1024 });
const { stdout } = await exec("systemctl", ["show", "baremetal-runner.service", "--property=MainPID", "--value"]);
const pid = Number(stdout.trim());
assert(Number.isInteger(pid) && pid > 1, "runner must be running");
const uid = (await exec("id", ["-u", "baremetal-runner"])).stdout.trim();
const gid = (await exec("id", ["-g", "baremetal-runner"])).stdout.trim();
const probe = `const fs=require('node:fs');const net=require('node:net');if(fs.existsSync('/etc/baremetal/web.env'))process.exit(2);const s=net.connect(27017,'127.0.0.1');s.once('connect',()=>process.exit(3));s.once('error',()=>process.exit(0));setTimeout(()=>process.exit(0),1500);`;
await exec("nsenter", ["--target", String(pid), "--mount", "--net", `--setuid=${uid}`, `--setgid=${gid}`, "/usr/bin/env", "node", "-e", probe]);
const cgroup = (await readFile(`/proc/${pid}/cgroup`, "utf8")).split("\n").find((line) => line.startsWith("0::"))!.slice(3);
assert(cgroup.endsWith("/supervisor"));
const jobs = path.join("/sys/fs/cgroup", cgroup.slice(0, -"/supervisor".length), "jobs");
const socket = process.env.RUNNER_SOCKET ?? "/run/baremetal-runner/runner.sock";
const REQUEST_MS = 75_000;
const send = (code?: string, signal?: AbortSignal) => new Promise<Record<string, unknown>>((resolve, reject) => {
  const timer = setTimeout(() => req.destroy(new Error("Runner host probe request timed out")), REQUEST_MS);
  const req = request({ socketPath: socket, signal, path: code ? "/v1/execute" : "/health", method: code ? "POST" : "GET", headers: { "Content-Type": "application/json" } }, (res) => {
    let data = "";
    res.on("data", (chunk) => {
      data += chunk;
      if (Buffer.byteLength(data) > 8 * 1024 * 1024) req.destroy(new Error("Runner host probe response exceeded its bound"));
    });
    res.on("end", () => { clearTimeout(timer); try { assert.equal(res.statusCode, 200); resolve(JSON.parse(data)); } catch (error) { reject(error); } });
    res.on("error", (error) => { clearTimeout(timer); reject(error); });
    res.on("aborted", () => { clearTimeout(timer); reject(new Error("Runner host probe response aborted")); });
  });
  req.on("error", (error) => { clearTimeout(timer); reject(error); });
  req.end(code ? JSON.stringify({ version: 1, operation: "run", exerciseId: "first-steps/hello-world", code, stdin: "", keys: ["b".repeat(64)] }) : undefined);
});

type Snapshot = { dir: string; pids: number[]; memoryEvents: string; pidsEvents: string; cpuStat: string; cpuMax: string; cpus: string };
const snapshots = async (): Promise<Snapshot[]> => {
  const result: Snapshot[] = [];
  for (const job of (await readdir(jobs)).filter((entry) => entry.startsWith("job-"))) {
    const dir = path.join(jobs, job);
    try {
      // Compiler cgroups have a different limit. Only count execution evidence.
      if ((await readFile(path.join(dir, "memory.max"), "utf8")).trim() !== "536870912") continue;
      assert.equal((await readFile(path.join(dir, "pids.max"), "utf8")).trim(), "128");
      result.push({ dir,
        pids: (await readFile(path.join(dir, "cgroup.procs"), "utf8")).trim().split(/\s+/).filter(Boolean).map(Number),
        memoryEvents: await readFile(path.join(dir, "memory.events"), "utf8"),
        pidsEvents: await readFile(path.join(dir, "pids.events"), "utf8"),
        cpuStat: await readFile(path.join(dir, "cpu.stat"), "utf8"),
        cpuMax: (await readFile(path.join(dir, "cpu.max"), "utf8")).trim(),
        cpus: (await readFile(path.join(dir, "cpuset.cpus.effective"), "utf8")).trim(),
      });
    } catch (error) { if ((error as NodeJS.ErrnoException).code !== "ENOENT") throw error; } // Job completed between reads.
  }
  return result;
};
const eventCount = (text: string, name: string) => Number(new RegExp(`^${name} ([0-9]+)$`, "m").exec(text)?.[1] ?? 0);
const awaitEmpty = async (milliseconds = 3000) => {
  const deadline = Date.now() + milliseconds;
  while ((await readdir(jobs)).length && Date.now() < deadline) await delay(10);
  assert.equal((await readdir(jobs)).length, 0, "all job cgroups must be removed");
};
const observe = async (code: string, visit: (sample: Snapshot, abort: AbortController) => Promise<void> | void, cancellation = false) => {
  await awaitEmpty(); // This validation host must have no unrelated learners/jobs.
  const abort = new AbortController();
  let finished = false;
  const response = send(code, abort.signal).then((value) => ({ value, error: undefined }), (error: unknown) => ({ value: undefined, error })).finally(() => { finished = true; });
  const deadline = Date.now() + REQUEST_MS;
  try {
    while (!finished && Date.now() < deadline) {
      for (const sample of await snapshots()) await visit(sample, abort);
      await delay(5);
    }
    assert(finished, "probe must finish before its deadline");
    const result = await response;
    if (cancellation) assert(abort.signal.aborted && result.error, "probe request must be cancelled while still running");
    else if (result.error) throw result.error;
    return result.value;
  } finally {
    abort.abort();
    await response;
    await awaitEmpty();
  }
};

let oom = false;
const oomBefore = eventCount(await readFile(path.join(jobs, "memory.events"), "utf8"), "oom_kill");
const memoryResult = await observe('#include <stdlib.h>\n#include <unistd.h>\n#include <sys/wait.h>\nint main(void){for(int i=0;i<20;i++){if(fork()==0){volatile char*p=malloc(64*1024*1024);if(p)for(int j=0;j<64*1024*1024;j+=4096)p[j]=1;sleep(3);_exit(0);}}while(wait(0)>0){}return 0;}', (sample) => {
  if (eventCount(sample.memoryEvents, "oom_kill") > 0) oom = true;
});
// OOM-group killing can remove a leaf before the next sample. The parent keeps
// hierarchical counters after child removal; the host is exclusively ours.
const oomAfter = eventCount(await readFile(path.join(jobs, "memory.events"), "utf8"), "oom_kill");
assert.equal(memoryResult?.phase, "run", "the memory probe must compile and reach execution");
assert(oom || oomAfter > oomBefore, "memory probe must trigger the execution cgroup OOM limit");
console.log(JSON.stringify({ event: "runner_host_probe", probe: "memory", leafOomObserved: oom, oomKillDelta: oomAfter - oomBefore }));

let processLimitEvents = 0;
const forkResult = await observe(`#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
int main(void){int limited=0;for(int i=0;i<160;i++){pid_t p=fork();if(p==0){sleep(2);_exit(0);}if(p<0){limited=(errno==EAGAIN);break;}}while(wait(0)>0){}printf("FORK_LIMIT=%d\\n",limited);return limited?0:1;}`, (sample) => {
  processLimitEvents = Math.max(processLimitEvents, eventCount(sample.pidsEvents, "max"));
});
assert(processLimitEvents > 0, "bounded fork probe must trigger pids.events max, not merely read pids.max");
assert.equal(forkResult?.ok, true, "fork probe must detect EAGAIN and reap its children");
assert.match(String(forkResult?.stdout), /FORK_LIMIT=1/);
console.log(JSON.stringify({ event: "runner_host_probe", probe: "pids", limitEvents: processLimitEvents }));

let throttledPeriods = 0, throttledUsec = 0;
const cpuCount = (list: string) => list.split(",").reduce((total, range) => { const [lo, hi = lo] = range.split("-").map(Number); return total + hi - lo + 1; }, 0);
const cpuResult = await observe(`#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
static double seconds(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;}
int main(void){for(int i=0;i<8;i++){pid_t p=fork();if(p<0)return 2;if(p==0){double end=seconds()+2;volatile uint64_t n=1;while(seconds()<end)n=n*1664525+1013904223;_exit(0);}}while(wait(0)>0){}return 0;}`, (sample) => {
  assert.equal(sample.cpuMax, "200000 100000", "execution quota must be two CPUs");
  assert(cpuCount(sample.cpus) > 2, "CPU enforcement validation requires at least three effective CPUs; a smaller host cannot demonstrate throttling above the two-CPU quota");
  throttledPeriods = Math.max(throttledPeriods, eventCount(sample.cpuStat, "nr_throttled"));
  throttledUsec = Math.max(throttledUsec, eventCount(sample.cpuStat, "throttled_usec"));
});
assert.equal(cpuResult?.ok, true, "bounded CPU probe must finish normally");
assert(throttledPeriods > 0 && throttledUsec > 0, "CPU probe must increase cpu.stat nr_throttled and throttled_usec; ancestor limits or host contention may require a dedicated host with more CPU capacity");

console.log(JSON.stringify({ event: "runner_host_probe", probe: "cpu", throttledPeriods, throttledUsec }));

type ProcessIdentity = { pid: number; start: string; session: number; state: string };
const identity = async (processId: number): Promise<ProcessIdentity | undefined> => {
  try {
    const raw = await readFile(`/proc/${processId}/stat`, "utf8");
    const fields = raw.slice(raw.lastIndexOf(")") + 2).trim().split(/\s+/);
    return { pid: processId, state: fields[0], session: Number(fields[3]), start: fields[19] };
  } catch (error) { if ((error as NodeJS.ErrnoException).code !== "ENOENT") throw error; }
};
const descendants: ProcessIdentity[] = [];
let cancellationAt = 0;
let runSeenAt = 0;
await observe(`#include <sys/types.h>
#include <unistd.h>
int main(void){int fd[2];if(pipe(fd))return 2;pid_t child=fork();if(child<0)return 3;if(child==0){close(fd[0]);if(setsid()<0)_exit(4);pid_t grandchild=fork();if(grandchild<0)_exit(5);if(grandchild==0){close(fd[1]);for(;;)pause();}char ready=1;if(write(fd[1],&ready,1)!=1)_exit(6);for(;;)pause();}close(fd[1]);char ready;if(read(fd[0],&ready,1)!=1)return 7;for(;;)pause();}`, async (sample, abort) => {
  if (!runSeenAt) runSeenAt = Date.now();
  if (abort.signal.aborted) return;
  const current: ProcessIdentity[] = [];
  for (const processId of sample.pids) {
    const executable = await readlink(`/proc/${processId}/exe`).catch(() => "");
    if (!executable.replace(/ \(deleted\)$/, "").endsWith("/prog")) continue;
    const process = await identity(processId);
    if (process && process.state !== "Z") current.push(process);
  }
  // Observe the learner parent, a new-session child and its grandchild before cancelling.
  if (current.length >= 3 && new Set(current.map((process) => process.session)).size >= 2) {
    assert(Date.now() - runSeenAt < 1500, "must cancel well before the engine's five-second timeout");
    descendants.push(...current);
    cancellationAt = Date.now();
    abort.abort();
  }
}, true);
assert(descendants.length >= 3, "must observe descendants including the setsid branch before cancellation");
assert(Date.now() - cancellationAt < 2500, "cancelled cgroup must disappear before the ordinary execution timeout");
for (const before of descendants) {
  const after = await identity(before.pid);
  assert(!after || after.start !== before.start || after.state === "Z", `cancelled descendant ${before.pid} must not remain running`);
}
console.log(JSON.stringify({ event: "runner_host_probe", probe: "cancellation", observedDescendants: descendants.length, cleanupMs: Date.now() - cancellationAt }));
assert.equal((await send('#include <stdio.h>\nint main(void){puts("healthy");}')).ok, true);
await awaitEmpty();
assert.equal((await send()).ready, true);
console.log("Runner secret/database isolation, OOM, process-count enforcement, CPU throttling, setsid descendant cancellation, cgroup cleanup and recovery passed.");
