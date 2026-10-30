// systemd entrypoint: keep the supervisor outside the delegated jobs subtree.
import { mkdir, readFile, writeFile } from "node:fs/promises";
import path from "node:path";
import { startRunner } from "./runner-service.mjs";

const entry = (await readFile("/proc/self/cgroup", "utf8")).split("\n").find((line) => line.startsWith("0::"));
if (!entry) throw new Error("Runner requires cgroup v2.");
const relative = entry.slice(3);
if (!relative.startsWith("/") || relative.split("/").includes("..")) throw new Error("Invalid delegated cgroup path.");
const root = path.join("/sys/fs/cgroup", relative);
const supervisor = path.join(root, "supervisor");
const jobs = path.join(root, "jobs");
await mkdir(supervisor, { recursive: true });
await mkdir(jobs, { recursive: true });
await writeFile(path.join(supervisor, "cgroup.procs"), String(process.pid));
await writeFile(path.join(root, "cgroup.subtree_control"), "+memory +pids +cpu +io");
await writeFile(path.join(jobs, "cgroup.subtree_control"), "+memory +pids +cpu +io");
process.env.RUNNER_CGROUP_ROOT = jobs;
if (!process.env.RUNNER_SOCKET) throw new Error("Set RUNNER_SOCKET.");
await startRunner(process.env.RUNNER_SOCKET);
console.log(JSON.stringify({ event: "runner_ready", version: 1, cgroups: true }));
