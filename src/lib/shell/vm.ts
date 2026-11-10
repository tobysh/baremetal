import { spawn, type ChildProcess } from "node:child_process";
import { randomBytes } from "node:crypto";
import { constants as fsConstants } from "node:fs";
import { chmod, chown, copyFile, link as link_, lstat, mkdir, mkdtemp, readdir, readFile, rm, rmdir, writeFile } from "node:fs/promises";
import { connect, type Socket } from "node:net";
import os from "node:os";
import path from "node:path";

/** One running microVM. `socketPath` is Firecracker's vsock proxy (a Unix socket on the host). */
export type Vm = {
  socketPath: string;
  /** Resolves when the VMM is gone, whoever ended it. */
  exited: Promise<void>;
  /** Destroy the VM and remove everything it left on the host. Safe to call twice. */
  destroy(): Promise<void>;
};

/**
 * Destroying a VM did not end it (or could not be confirmed): a process may still run under its uid, and its chroot,
 * which that uid owns, was left alone. The caller must keep the VM's slot and uid out of use until a restart reaps it.
 */
export class VmLeakError extends Error {}

export type VmSpec = {
  firecracker: string;
  kernel: string;
  /** Shared, read-only image. Never written by the guest. */
  rootfs: string;
  vcpus?: number;
  memMib?: number;
};

export const GUEST_PORT = 5000;
const BOOT_ARGS = "console=ttyS0 quiet loglevel=0 reboot=k panic=1 pci=off ro init=/init random.trust_cpu=on";

const vmConfig = (spec: { kernel: string; rootfs: string; vcpus?: number; memMib?: number }, vsockPath: string) =>
  JSON.stringify({
    "boot-source": { kernel_image_path: spec.kernel, boot_args: BOOT_ARGS },
    drives: [{ drive_id: "rootfs", path_on_host: spec.rootfs, is_root_device: true, is_read_only: true }],
    "machine-config": { vcpu_count: spec.vcpus ?? 1, mem_size_mib: spec.memMib ?? 256, smt: false },
    "network-interfaces": [],
    vsock: { guest_cid: 3, uds_path: vsockPath },
  });

const childExited = (child: ChildProcess) =>
  new Promise<void>((resolve) => {
    if (child.exitCode !== null || child.signalCode !== null) return resolve();
    child.once("close", () => resolve());
    child.once("error", () => resolve());
  });

/**
 * Development and test driver: Firecracker as the calling user, no jailer, no cgroup limits.
 * Not a security boundary; the broker refuses to use it when NODE_ENV=production.
 */
export async function startUnjailedVm(spec: VmSpec): Promise<Vm> {
  const dir = await mkdtemp(path.join(os.tmpdir(), "bm-vm-"));
  const socketPath = path.join(dir, "v.sock");
  await writeFile(path.join(dir, "vm.json"), vmConfig(spec, socketPath));
  const child = spawn(spec.firecracker, ["--no-api", "--config-file", path.join(dir, "vm.json"), "--level", "Error"], { stdio: "ignore", detached: false });
  const exited = childExited(child);
  let destroyed: Promise<void> | undefined;
  return {
    socketPath,
    exited,
    destroy: () => (destroyed ??= (async () => {
      child.kill("SIGKILL");
      await exited;
      await rm(dir, { recursive: true, force: true });
    })()),
  };
}

export type JailedSpec = VmSpec & {
  jailer: string;
  /** Root-owned directory that holds the per-VM chroots. */
  chrootBase: string;
  /** Unprivileged uid/gid for this VM's VMM; a dedicated number with no account behind it, never shared by two live VMs. */
  uid: number;
  /**
   * Group of the unprivileged broker. When set, the vsock socket is exposed as `<chrootBase>/sockets/<id>.sock`, a hard
   * link owned by root with this group and mode 0660, so the broker can connect without traversing the chroot and the
   * VMM uid cannot change who may connect. Without it the socket stays inside the chroot (root-only use).
   */
  socketGid?: number;
  cgroupRoot?: string;
  cpuMax?: string;
  memoryMax?: string;
  pidsMax?: number;
  /** How long destroy() waits for cgroup.kill to empty the cgroup. */
  killWaitMs?: number;
};

const sleep = (ms: number) => new Promise((r) => setTimeout(r, ms));
export const socketsDir = (chrootBase: string) => path.join(chrootBase, "sockets");

/**
 * Production driver: the Firecracker jailer (chroot, dedicated uid, new pid namespace, cgroup v2 limits).
 * Needs root to start, so only the launcher (src/lib/shell/launcher.ts) calls it. The jailer's parent exits immediately
 * with --new-pid-ns, so the VM is tracked by its cgroup (cgroup.procs) and destroyed with cgroup.kill.
 * See docs/interactive-shell-isolation.md.
 */
export async function startJailedVm(spec: JailedSpec): Promise<Vm> {
  const cgroupRoot = spec.cgroupRoot ?? "/sys/fs/cgroup";
  // The jailer names its chroot and cgroup after the exec file's name.
  const name = path.basename(spec.firecracker);
  const id = `bm-${randomBytes(6).toString("hex")}`;
  const vmDir = path.join(spec.chrootBase, name, id);
  const root = path.join(vmDir, "root");
  const cgroup = path.join(cgroupRoot, name, id);
  const link = spec.socketGid === undefined ? undefined : path.join(socketsDir(spec.chrootBase), `${id}.sock`);
  const alive = async () => (await readFile(path.join(cgroup, "cgroup.procs"), "utf8").catch(() => "")).trim().length > 0;
  let destroyed: Promise<void> | undefined;
  const destroy = () => (destroyed ??= (async () => {
    await writeFile(path.join(cgroup, "cgroup.kill"), "1").catch(() => {});
    const deadline = Date.now() + (spec.killWaitMs ?? 5_000);
    while ((await alive()) && Date.now() < deadline) await sleep(100);
    // Nothing is deleted while a process may still run as the VM uid: it owns the chroot and could race the removal.
    // The caller must not reuse this VM's uid either; the launcher keeps the slot until a restart reaps it.
    if (await alive()) throw new VmLeakError(`VM ${id} survived cgroup.kill`);
    if (link) await rm(link, { force: true });
    await rm(vmDir, { recursive: true, force: true });
    await rmdir(cgroup).catch(() => {}); // cgroup directories are removed with rmdir, not rm
  })());
  let socketPath = path.join(root, "v.sock");
  try {
    await mkdir(root, { recursive: true });
    // Reflink where the filesystem supports it (XFS, btrfs), a plain copy otherwise. Sharing blocks is safe: the drive
    // is attached read-only (is_read_only in vmConfig, "ro" on the kernel command line), and a write to a reflinked
    // copy would be copy-on-write anyway, never reaching the source.
    await copyFile(spec.kernel, path.join(root, "vmlinux"), fsConstants.COPYFILE_FICLONE);
    await copyFile(spec.rootfs, path.join(root, "rootfs.ext4"), fsConstants.COPYFILE_FICLONE);
    await writeFile(path.join(root, "vm.json"), vmConfig({ ...spec, kernel: "/vmlinux", rootfs: "/rootfs.ext4" }, "/v.sock"));
    for (const entry of ["", "root", "root/vmlinux", "root/rootfs.ext4", "root/vm.json"]) await chown(path.join(vmDir, entry), spec.uid, spec.uid);
    const jailer = spawn(spec.jailer, [
      "--id", id, "--exec-file", spec.firecracker, "--uid", String(spec.uid), "--gid", String(spec.uid),
      "--chroot-base-dir", spec.chrootBase, "--cgroup-version", "2", "--new-pid-ns",
      "--cgroup", `cpu.max=${spec.cpuMax ?? "50000 100000"}`, "--cgroup", `memory.max=${spec.memoryMax ?? "400M"}`, "--cgroup", `pids.max=${spec.pidsMax ?? 64}`,
      "--", "--no-api", "--config-file", "/vm.json", "--level", "Error",
    ], { stdio: "ignore" });
    await childExited(jailer);
    if (!(await alive())) throw new Error("jailer did not start a VM");
    if (link) socketPath = await exposeSocket(path.join(root, "v.sock"), link, spec.uid, spec.socketGid!);
  } catch (error) {
    // A VM that could not be destroyed must surface as a leak, never as an ordinary start failure, or the launcher
    // would hand its slot (and uid) to the next VM while it may still be running.
    await destroy().catch((leak: unknown) => {
      throw leak instanceof VmLeakError ? leak : new VmLeakError(`VM ${id} cleanup failed: ${String(leak)}`);
    });
    throw error;
  }
  const exited = (async () => {
    while (await alive()) await sleep(200);
  })();
  return { socketPath, exited, destroy };
}

/**
 * Give the broker's group access to a VM's vsock socket. The chroot belongs to the VMM uid, so nothing in it is trusted:
 * the socket is hard-linked into a root-owned directory first (link(2) does not follow symlinks), and only the link,
 * which the VMM cannot replace, is checked and re-owned. Connecting to a Unix socket needs write access to its inode,
 * so root:<broker group> 0660 lets the broker in and keeps the VMM uid from widening it (it no longer owns the inode).
 */
async function exposeSocket(inChroot: string, link: string, vmUid: number, gid: number): Promise<string> {
  // Firecracker binds the vsock socket while setting up devices, just after it starts.
  for (let i = 0; i < 100; i++) {
    const st = await lstat(inChroot).catch(() => null);
    if (st) break;
    await sleep(20);
  }
  await link_(inChroot, link);
  const st = await lstat(link);
  if (!st.isSocket() || st.uid !== vmUid || st.nlink !== 2) {
    await rm(link, { force: true });
    throw new Error("vsock socket is not the VMM's socket");
  }
  await chown(link, 0, gid);
  await chmod(link, 0o660);
  return link;
}

/**
 * Remove VMs, chroots, cgroups and socket links left by a crashed launcher. Run once at startup, before accepting
 * requests: the launcher is their only owner, so anything here is stale. A VM that does not die within `waitMs` keeps
 * its files and the call throws VmLeakError: starting anyway could hand its uid to a new VM.
 */
export async function reapStaleVms(chrootBase: string, firecracker: string, cgroupRoot = "/sys/fs/cgroup", waitMs = 5_000): Promise<number> {
  const name = path.basename(firecracker);
  const list = async (dir: string) => (await readdir(dir).catch(() => [] as string[])).filter((i) => /^bm-[0-9a-f]{12}$/.test(i));
  const ids = new Set([...(await list(path.join(chrootBase, name))), ...(await list(path.join(cgroupRoot, name)))]);
  const alive = async (id: string) => (await readFile(path.join(cgroupRoot, name, id, "cgroup.procs"), "utf8").catch(() => "")).trim().length > 0;
  for (const id of ids) await writeFile(path.join(cgroupRoot, name, id, "cgroup.kill"), "1").catch(() => {});
  let survivors = [...ids];
  const deadline = Date.now() + waitMs;
  for (;;) {
    const next: string[] = [];
    for (const id of survivors) if (await alive(id)) next.push(id);
    survivors = next;
    if (!survivors.length || Date.now() >= deadline) break;
    await sleep(100);
  }
  for (const id of ids) {
    if (survivors.includes(id)) continue;
    await rm(path.join(chrootBase, name, id), { recursive: true, force: true });
    await rmdir(path.join(cgroupRoot, name, id)).catch(() => {});
  }
  for (const entry of await readdir(socketsDir(chrootBase)).catch(() => [] as string[])) {
    const m = /^(bm-[0-9a-f]{12})\.sock$/.exec(entry);
    if (m && !survivors.includes(m[1])) await rm(path.join(socketsDir(chrootBase), entry), { force: true });
  }
  if (survivors.length) throw new VmLeakError(`stale VMs survived cgroup.kill: ${survivors.join(", ")}`);
  return ids.size;
}

/**
 * Connect to the guest agent through Firecracker's vsock proxy. The guest is still booting, so retry until it
 * listens or the deadline passes. Resolves with the socket and any bytes that arrived after the handshake line.
 */
export async function connectGuest(socketPath: string, deadline: number, aborted: () => boolean): Promise<{ socket: Socket; rest: Buffer }> {
  while (Date.now() < deadline && !aborted()) {
    const attempt = await new Promise<{ socket: Socket; rest: Buffer } | null>((resolve) => {
      const socket = connect(socketPath);
      let buf = Buffer.alloc(0);
      const fail = () => { socket.destroy(); resolve(null); };
      socket.once("error", fail);
      socket.once("close", fail);
      socket.once("connect", () => socket.write(`CONNECT ${GUEST_PORT}\n`));
      const onData = (chunk: Buffer) => {
        buf = Buffer.concat([buf, chunk]);
        const nl = buf.indexOf(10);
        if (nl < 0) { if (buf.length > 64) fail(); return; }
        socket.off("data", onData);
        socket.off("close", fail);
        socket.off("error", fail);
        socket.on("error", () => {}); // the caller attaches its own handlers; never let an early error throw
        if (buf.subarray(0, nl).toString().startsWith("OK ")) resolve({ socket, rest: buf.subarray(nl + 1) });
        else fail();
      };
      socket.on("data", onData);
    });
    if (attempt) return attempt;
    await new Promise((r) => setTimeout(r, 50));
  }
  throw new Error("guest agent did not come up");
}
