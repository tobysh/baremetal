// Interactive-shell broker (unprivileged): holds microVM leases from the root launcher and relays framed terminal bytes. Design and threat model:
// docs/interactive-shell-isolation.md. Runs in its own service; it must hold no web, database or API secrets.
import { chmod, lstat, rm } from "node:fs/promises";
import { createBroker, type BrokerLimits } from "../src/lib/shell/broker";
import { launcherVmStarter } from "../src/lib/shell/launcher-client";
import { startUnjailedVm, type Vm } from "../src/lib/shell/vm";

const need = (name: string) => {
  const value = process.env[name];
  if (!value) throw new Error(`${name} is required`);
  return value;
};
const num = (name: string, fallback: number) => {
  const raw = process.env[name];
  if (raw === undefined) return fallback;
  const n = Number(raw);
  if (!Number.isFinite(n) || n <= 0) throw new Error(`${name} must be a positive number`);
  return n;
};
const socketEnv = (name: string) => {
  const value = need(name);
  if (!value.startsWith("/") || Buffer.byteLength(value) > 100) throw new Error(`${name} must be an absolute path of at most 100 bytes`);
  return value;
};

// The broker parses traffic from hostile guests, so it never runs as root. VMs come from the root launcher
// (scripts/shell-launcher.mts) over its private socket; the broker's group can reach each VM's vsock socket.
if (process.getuid?.() === 0) throw new Error("the shell broker must not run as root; run scripts/shell-launcher.mts as root and point SHELL_LAUNCHER_SOCKET at it");
const socketPath = socketEnv("SHELL_BROKER_SOCKET");
const viaLauncher = process.env.SHELL_MODE !== "unjailed";
if (!viaLauncher && process.env.NODE_ENV === "production") throw new Error("SHELL_MODE=unjailed is for development only and is refused in production");

let startVm: () => Promise<Vm>;
if (viaLauncher) {
  startVm = launcherVmStarter(socketEnv("SHELL_LAUNCHER_SOCKET"));
} else {
  console.warn("WARNING: unjailed development mode. Guests are NOT isolated by the jailer.");
  const spec = { firecracker: need("FIRECRACKER"), kernel: need("SHELL_KERNEL"), rootfs: need("SHELL_ROOTFS") };
  startVm = () => startUnjailedVm(spec);
}

const limits: Partial<BrokerLimits> = {
  maxSessions: num("SHELL_MAX_SESSIONS", 4),
  idleMs: num("SHELL_IDLE_SECONDS", 300) * 1000,
  absoluteMs: num("SHELL_MAX_SECONDS", 1800) * 1000,
  outputPerSec: num("SHELL_OUTPUT_PER_SEC", 128 * 1024),
  outputBurst: num("SHELL_OUTPUT_BURST", 512 * 1024),
  outputFloodMs: num("SHELL_OUTPUT_FLOOD_SECONDS", 60) * 1000,
};
const broker = createBroker({ startVm, limits });

// Only remove a leftover socket, never anything else at that path.
const existing = await lstat(socketPath).catch(() => null);
if (existing) {
  if (!existing.isSocket()) throw new Error(`${socketPath} exists and is not a socket`);
  await rm(socketPath);
}
await new Promise<void>((resolve, reject) => broker.server.once("error", reject).listen(socketPath, resolve));
await chmod(socketPath, 0o660); // group access for the web service only; set the group in the service unit
console.log(JSON.stringify({ event: "listening", socketPath, mode: viaLauncher ? "launcher" : "unjailed", limits }));

for (const signal of ["SIGTERM", "SIGINT"] as const) {
  process.once(signal, () => {
    void broker.shutdown().finally(() => process.exit(0));
  });
}
