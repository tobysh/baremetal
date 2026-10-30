import { spawn, type ChildProcess } from "node:child_process";
import { randomBytes } from "node:crypto";
import { copyFile, chown, mkdir, mkdtemp, readdir, readFile, rm, rmdir, writeFile } from "node:fs/promises";
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
  /** Unprivileged uid/gid for the VMM; a dedicated number with no account behind it. */
  uid: number;
  cgroupRoot?: string;
  cpuMax?: string;
  memoryMax?: string;
  pidsMax?: number;
};

/**
 * Production driver: the Firecracker jailer (chroot, dedicated uid, new pid namespace, cgroup v2 limits).
 * Needs root to start. The jailer's parent exits immediately with --new-pid-ns, so the VM is tracked by its
 * cgroup (cgroup.procs) and destroyed with cgroup.kill. See docs/interactive-shell-isolation.md.
 */
export async function startJailedVm(spec: JailedSpec): Promise<Vm> {
  const cgroupRoot = spec.cgroupRoot ?? "/sys/fs/cgroup";
  // The jailer names its chroot and cgroup after the exec file's name.
  const name = path.basename(spec.firecracker);
  const id = `bm-${randomBytes(6).toString("hex")}`;
  const vmDir = path.join(spec.chrootBase, name, id);
  const root = path.join(vmDir, "root");
  const cgroup = path.join(cgroupRoot, name, id);
  const alive = async () => (await readFile(path.join(cgroup, "cgroup.procs"), "utf8").catch(() => "")).trim().length > 0;
  let destroyed: Promise<void> | undefined;
  const destroy = () => (destroyed ??= (async () => {
    await writeFile(path.join(cgroup, "cgroup.kill"), "1").catch(() => {});
    for (let i = 0; i < 50 && (await alive()); i++) await new Promise((r) => setTimeout(r, 100));
    await rm(vmDir, { recursive: true, force: true });
    await rmdir(cgroup).catch(() => {}); // cgroup directories are removed with rmdir, not rm
  })());
  try {
    await mkdir(root, { recursive: true });
    await copyFile(spec.kernel, path.join(root, "vmlinux"));
    await copyFile(spec.rootfs, path.join(root, "rootfs.ext4"));
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
  } catch (error) {
    await destroy();
    throw error;
  }
  const exited = (async () => {
    while (await alive()) await new Promise((r) => setTimeout(r, 200));
  })();
  return { socketPath: path.join(root, "v.sock"), exited, destroy };
}

/** Remove chroots and cgroups left by a crashed broker. Run once at startup, before accepting sessions. */
export async function reapStaleVms(chrootBase: string, firecracker: string, cgroupRoot = "/sys/fs/cgroup"): Promise<number> {
  const name = path.basename(firecracker);
  const ids = await readdir(path.join(chrootBase, name)).catch(() => [] as string[]);
  let reaped = 0;
  for (const id of ids.filter((i) => i.startsWith("bm-"))) {
    await writeFile(path.join(cgroupRoot, name, id, "cgroup.kill"), "1").catch(() => {});
    await new Promise((r) => setTimeout(r, 300));
    await rm(path.join(chrootBase, name, id), { recursive: true, force: true });
    await rmdir(path.join(cgroupRoot, name, id)).catch(() => {});
    reaped++;
  }
  return reaped;
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
