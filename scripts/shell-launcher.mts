// Interactive-shell launcher: the only privileged part of the shell service. It creates and destroys microVMs for the
// unprivileged broker over a private Unix socket and does nothing else (no network, no learner data, no guest traffic).
// Protocol and lease rules: src/lib/shell/launcher.ts. Design: docs/interactive-shell-isolation.md.
//
// Jailed (production, root):  SHELL_LAUNCHER_SOCKET SHELL_BROKER_GID FIRECRACKER JAILER SHELL_KERNEL SHELL_ROOTFS
//                             SHELL_CHROOT_BASE [SHELL_VM_UID_BASE=300000] [SHELL_MAX_VMS=4] [SHELL_VM_MAX_SECONDS=2100]
// Unjailed (development only, any user): SHELL_LAUNCHER_MODE=unjailed SHELL_LAUNCHER_SOCKET FIRECRACKER SHELL_KERNEL SHELL_ROOTFS
//
// Permissions: the socket is created under umask 0177, then given to root:SHELL_BROKER_GID with mode 0660, so only
// root and the broker's group can connect. Node cannot read SO_PEERCRED, so the peer is not checked per connection;
// the socket mode and its parent directory (root-owned, not group/world-writable, checked below) are the gate.
import { chmod, chown, lstat, mkdir, realpath, rm, stat } from "node:fs/promises";
import path from "node:path";
import { createLauncher } from "../src/lib/shell/launcher";
import { reapStaleVms, socketsDir, startJailedVm, startUnjailedVm, type Vm } from "../src/lib/shell/vm";

const need = (name: string) => {
  const value = process.env[name];
  if (!value) throw new Error(`${name} is required`);
  return value;
};
const int = (name: string, fallback: number, min: number, max: number) => {
  const raw = process.env[name];
  const n = raw === undefined ? fallback : Number(raw);
  if (!Number.isInteger(n) || n < min || n > max) throw new Error(`${name} must be an integer from ${min} to ${max}`);
  return n;
};
/** A fixed input the launcher hands to the jailer as root: resolved once, owned by root, writable by nobody else. */
async function trusted(name: string, kind: "file" | "dir", requireRoot: boolean) {
  const value = need(name);
  if (!path.isAbsolute(value)) throw new Error(`${name} must be an absolute path`);
  const resolved = await realpath(value);
  const st = await stat(resolved);
  if (kind === "file" ? !st.isFile() : !st.isDirectory()) throw new Error(`${name} must be a ${kind}`);
  if (requireRoot && (st.uid !== 0 || st.mode & 0o022)) throw new Error(`${name} must be owned by root and not writable by group or others`);
  return resolved;
}

const socketPath = need("SHELL_LAUNCHER_SOCKET");
if (!path.isAbsolute(socketPath) || Buffer.byteLength(socketPath) > 100) throw new Error("SHELL_LAUNCHER_SOCKET must be an absolute path of at most 100 bytes");
const jailed = process.env.SHELL_LAUNCHER_MODE !== "unjailed";
if (!jailed && process.env.NODE_ENV === "production") throw new Error("SHELL_LAUNCHER_MODE=unjailed is for development only and is refused in production");
if (jailed && process.getuid?.() !== 0) throw new Error("the jailed launcher must run as root (it starts the jailer)");

const maxVms = int("SHELL_MAX_VMS", 4, 1, 64);
const maxLifetimeMs = int("SHELL_VM_MAX_SECONDS", 2100, 10, 24 * 3600) * 1000;
const firecracker = await trusted("FIRECRACKER", "file", jailed);
const kernel = await trusted("SHELL_KERNEL", "file", jailed);
const rootfs = await trusted("SHELL_ROOTFS", "file", jailed);

let startVm: (slot: number) => Promise<Vm>;
let brokerGid: number | undefined;
if (jailed) {
  brokerGid = int("SHELL_BROKER_GID", -1, 1, 2 ** 31 - 1);
  const uidBase = int("SHELL_VM_UID_BASE", 300000, 65536, 2 ** 31 - 1 - 64);
  const jailer = await trusted("JAILER", "file", true);
  const chrootBase = await trusted("SHELL_CHROOT_BASE", "dir", true);
  // The broker reaches vsock sockets through <chrootBase>/sockets, so it must be able to search chrootBase.
  const base = await stat(chrootBase);
  if (!(base.mode & 0o001) && !(base.gid === brokerGid && base.mode & 0o010)) throw new Error("SHELL_CHROOT_BASE must be searchable by the broker (mode 0711, or group SHELL_BROKER_GID with g+x)");
  // root:<broker group> 0750: the broker can open sockets in it, VMM uids cannot even list it.
  const sockets = socketsDir(chrootBase);
  await mkdir(sockets, { recursive: true, mode: 0o750 });
  const sdir = await lstat(sockets);
  if (!sdir.isDirectory() || sdir.uid !== 0) throw new Error(`${sockets} must be a root-owned directory`);
  await chown(sockets, 0, brokerGid);
  await chmod(sockets, 0o750);
  const reaped = await reapStaleVms(chrootBase, firecracker);
  if (reaped) console.log(JSON.stringify({ event: "reaped_stale_vms", count: reaped }));
  // One uid per slot: two live VMMs never share a uid, so a compromised VMM cannot signal or ptrace another.
  startVm = (slot) => startJailedVm({ firecracker, kernel, rootfs, jailer, chrootBase, uid: uidBase + slot, socketGid: brokerGid });
} else {
  console.warn("WARNING: unjailed development launcher. Guests are NOT isolated by the jailer.");
  startVm = () => startUnjailedVm({ firecracker, kernel, rootfs });
}

// The socket's directory decides who can replace or reach it; refuse one that others can write to.
const parent = await stat(path.dirname(socketPath));
if (parent.mode & 0o002 && !(parent.mode & 0o1000)) throw new Error("SHELL_LAUNCHER_SOCKET must not be in a world-writable directory");
if (jailed && (parent.uid !== 0 || parent.mode & 0o022)) throw new Error("SHELL_LAUNCHER_SOCKET's directory must be owned by root and not writable by group or others");

const launcher = createLauncher({ startVm, limits: { maxVms, maxLifetimeMs } });
const existing = await lstat(socketPath).catch(() => null);
if (existing) {
  if (!existing.isSocket()) throw new Error(`${socketPath} exists and is not a socket`);
  await rm(socketPath);
}
// The socket is born 0600, so there is no window where anyone else can connect. The umask is restored before any VM
// is created: chroot directories need their usual modes.
const umask = process.umask(0o177);
await new Promise<void>((resolve, reject) => launcher.server.once("error", reject).listen(socketPath, resolve));
process.umask(umask);
if (brokerGid !== undefined) {
  await chown(socketPath, 0, brokerGid);
  await chmod(socketPath, 0o660);
}
console.log(JSON.stringify({ event: "listening", socketPath, mode: jailed ? "jailed" : "unjailed", maxVms, maxLifetimeMs }));

for (const signal of ["SIGTERM", "SIGINT"] as const) {
  process.once(signal, () => {
    void launcher.shutdown().finally(() => process.exit(0));
  });
}
