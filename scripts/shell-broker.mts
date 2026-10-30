// Interactive-shell broker: owns microVM lifetimes and relays framed terminal bytes. Design and threat model:
// docs/interactive-shell-isolation.md. Runs in its own service; it must hold no web, database or API secrets.
import { chmod, lstat, rm } from "node:fs/promises";
import { createBroker, type BrokerLimits } from "../src/lib/shell/broker";
import { reapStaleVms, startJailedVm, startUnjailedVm, type Vm } from "../src/lib/shell/vm";

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

const socketPath = need("SHELL_BROKER_SOCKET");
if (!socketPath.startsWith("/") || Buffer.byteLength(socketPath) > 100) throw new Error("SHELL_BROKER_SOCKET must be an absolute path of at most 100 bytes");
const spec = { firecracker: need("FIRECRACKER"), kernel: need("SHELL_KERNEL"), rootfs: need("SHELL_ROOTFS") };
const jailed = process.env.SHELL_MODE !== "unjailed";
if (!jailed && process.env.NODE_ENV === "production") throw new Error("SHELL_MODE=unjailed is for development only and is refused in production");

let startVm: () => Promise<Vm>;
if (jailed) {
  if (process.getuid?.() !== 0) throw new Error("jailed mode needs root to start the jailer (see the design doc on privilege separation)");
  const jailedSpec = { ...spec, jailer: need("JAILER"), chrootBase: need("SHELL_CHROOT_BASE"), uid: num("SHELL_VM_UID", 300001) };
  const reaped = await reapStaleVms(jailedSpec.chrootBase, spec.firecracker);
  if (reaped) console.log(JSON.stringify({ event: "reaped_stale_vms", count: reaped }));
  startVm = () => startJailedVm(jailedSpec);
} else {
  console.warn("WARNING: unjailed development mode. Guests are NOT isolated by the jailer.");
  startVm = () => startUnjailedVm(spec);
}

const limits: Partial<BrokerLimits> = {
  maxSessions: num("SHELL_MAX_SESSIONS", 4),
  idleMs: num("SHELL_IDLE_SECONDS", 300) * 1000,
  absoluteMs: num("SHELL_MAX_SECONDS", 1800) * 1000,
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
console.log(JSON.stringify({ event: "listening", socketPath, mode: jailed ? "jailed" : "unjailed", limits }));

for (const signal of ["SIGTERM", "SIGINT"] as const) {
  process.once(signal, () => {
    void broker.shutdown().finally(() => process.exit(0));
  });
}
