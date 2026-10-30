// Run only on the disposable deployment validation host, as root.
import assert from "node:assert/strict";
import { execFile } from "node:child_process";
import { promisify } from "node:util";
import { readFile, readdir, stat } from "node:fs/promises";
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
const exec = promisify(execFile);
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
const send = (code?: string) => new Promise<Record<string, unknown>>((resolve, reject) => {
  const req = request({ socketPath: socket, path: code ? "/v1/execute" : "/health", method: code ? "POST" : "GET", headers: { "Content-Type": "application/json" } }, (res) => {
    let data = ""; res.on("data", (chunk) => { data += chunk; });
    res.on("end", () => { try { assert.equal(res.statusCode, 200); resolve(JSON.parse(data)); } catch (error) { reject(error); } });
    res.on("error", reject);
  });
  req.on("error", reject);
  req.end(code ? JSON.stringify({ version: 1, operation: "run", exerciseId: "first-steps/hello-world", code, stdin: "", keys: ["b".repeat(64)] }) : undefined);
});
let oom = false, executionPidsLimit = false;
let finished = false;
const bomb = send('#include <stdlib.h>\n#include <unistd.h>\n#include <sys/wait.h>\nint main(void){for(int i=0;i<20;i++){if(fork()==0){volatile char*p=malloc(64*1024*1024);if(p)for(int j=0;j<64*1024*1024;j+=4096)p[j]=1;sleep(3);_exit(0);}}while(wait(0)>0){}return 0;}').finally(() => { finished = true; });
while (!finished) {
  for (const job of await readdir(jobs)) {
    const dir = path.join(jobs, job);
    const events = await readFile(path.join(dir, "memory.events"), "utf8").catch(() => "");
    if (/^oom_kill [1-9]/m.test(events)) oom = true;
    if ((await readFile(path.join(dir, "pids.max"), "utf8").catch(() => "")).trim() === "128") executionPidsLimit = true;
  }
  await delay(5);
}
await bomb;
assert(oom, "memory probe must trigger the execution cgroup OOM limit");
assert(executionPidsLimit, "execution pids.max must be 128");
assert.equal((await send('#include <stdio.h>\nint main(void){puts("healthy");}')).ok, true);
for (let i = 0; i < 100 && (await readdir(jobs)).length; i++) await delay(20);
assert.equal((await readdir(jobs)).length, 0, "all job cgroups must be removed");
assert.equal((await send()).ready, true);
console.log("Runner cannot reach web secrets or MongoDB; memory limit, process configuration, cleanup and recovery passed.");
