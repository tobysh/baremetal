import test from "node:test";
import assert from "node:assert/strict";
import { chmod, mkdir, mkdtemp, readdir, rm, stat, writeFile } from "node:fs/promises";
import { connect } from "node:net";
import os from "node:os";
import path from "node:path";
import { createLauncher } from "../../src/lib/shell/launcher";
import { chrootBaseProblem, launcherSocketDirProblem } from "../../src/lib/shell/launcher-checks";
import { reapStaleVms, startJailedVm, VmLeakError, type Vm } from "../../src/lib/shell/vm";

const GID = 4242;
const dirStat = (uid: number, gid: number, mode: number) => ({ uid, gid, mode: 0o040000 | mode });

test("the launcher socket directory must belong to the broker's group", () => {
  assert.equal(launcherSocketDirProblem(dirStat(0, GID, 0o750), { jailed: true, brokerGid: GID }), null);
  assert.match(launcherSocketDirProblem(dirStat(0, GID + 1, 0o750), { jailed: true, brokerGid: GID }) ?? "", /SHELL_BROKER_GID/);
  assert.match(launcherSocketDirProblem(dirStat(0, 0, 0o750), { jailed: true, brokerGid: GID }) ?? "", /SHELL_BROKER_GID/);
  assert.match(launcherSocketDirProblem(dirStat(1000, GID, 0o750), { jailed: true, brokerGid: GID }) ?? "", /owned by root/);
  assert.match(launcherSocketDirProblem(dirStat(0, GID, 0o770), { jailed: true, brokerGid: GID }) ?? "", /writable/);
  assert.match(launcherSocketDirProblem(dirStat(0, GID, 0o757), { jailed: true, brokerGid: GID }) ?? "", /writable/);
  assert.match(launcherSocketDirProblem(dirStat(0, GID, 0o750), { jailed: true }) ?? "", /SHELL_BROKER_GID/);
  // Development (unjailed) launcher: only a world-writable, non-sticky directory is refused.
  assert.equal(launcherSocketDirProblem(dirStat(1000, 1000, 0o755), { jailed: false }), null);
  assert.equal(launcherSocketDirProblem(dirStat(0, 0, 0o1777), { jailed: false }), null);
  assert.match(launcherSocketDirProblem(dirStat(0, 0, 0o777), { jailed: false }) ?? "", /world-writable/);
});

test("the chroot base must be searchable by the broker", () => {
  assert.equal(chrootBaseProblem(dirStat(0, 0, 0o711), GID), null);
  assert.equal(chrootBaseProblem(dirStat(0, GID, 0o710), GID), null);
  assert.match(chrootBaseProblem(dirStat(0, 0, 0o700), GID) ?? "", /searchable/);
  assert.match(chrootBaseProblem(dirStat(0, 0, 0o710), GID) ?? "", /searchable/);
});

async function launcherRound(startVm: (slot: number) => Promise<Vm>, maxVms = 1) {
  const dir = await mkdtemp(path.join(os.tmpdir(), "bm-launcher-test-"));
  const sock = path.join(dir, "l.sock");
  const launcher = createLauncher({ startVm, limits: { maxVms }, log: () => {} });
  await new Promise<void>((r) => launcher.server.listen(sock, r));
  const request = () =>
    new Promise<string>((resolve, reject) => {
      const c = connect(sock);
      let got = "";
      c.on("connect", () => c.write('{"op":"create"}\n'));
      c.on("data", (d) => (got += d.toString()));
      c.on("close", () => resolve(got.trim()));
      c.on("error", reject);
    });
  return {
    launcher,
    request,
    done: async () => {
      await launcher.shutdown();
      await rm(dir, { recursive: true, force: true });
    },
  };
}

test("a VM that may have survived a failed start keeps its slot (and uid) reserved", async () => {
  const leak = await launcherRound(async () => {
    throw new VmLeakError("survived");
  });
  assert.match(await leak.request(), /start_failed/);
  await new Promise((r) => setTimeout(r, 20));
  assert.equal(leak.launcher.stats().active, 1);
  assert.match(await leak.request(), /busy/);
  await leak.done();

  const clean = await launcherRound(async () => {
    throw new Error("jailer did not start a VM");
  });
  assert.match(await clean.request(), /start_failed/);
  await new Promise((r) => setTimeout(r, 20));
  assert.equal(clean.launcher.stats().active, 0);
  await clean.done();
});

async function fakeHost() {
  const dir = await mkdtemp(path.join(os.tmpdir(), "bm-jail-test-"));
  const base = path.join(dir, "jail");
  const cg = path.join(dir, "cgroup");
  await mkdir(path.join(base, "firecracker"), { recursive: true });
  await mkdir(path.join(base, "sockets"), { recursive: true });
  await mkdir(path.join(cg, "firecracker"), { recursive: true });
  /** A VM directory and cgroup; `procs` is what cgroup.procs holds (non-empty means a process is still alive). */
  const vm = async (id: string, procs: string) => {
    await mkdir(path.join(base, "firecracker", id, "root"), { recursive: true });
    await writeFile(path.join(base, "firecracker", id, "root", "rootfs.ext4"), "x");
    await mkdir(path.join(cg, "firecracker", id), { recursive: true });
    await writeFile(path.join(cg, "firecracker", id, "cgroup.procs"), procs);
    await writeFile(path.join(base, "sockets", `${id}.sock`), "");
  };
  return { dir, base, cg, vm, cleanup: () => rm(dir, { recursive: true, force: true }) };
}

test("the reaper never deletes the files of a VM that is still alive, and refuses to continue", async () => {
  const host = await fakeHost();
  await host.vm("bm-aaaaaaaaaaaa", "4242\n");
  await host.vm("bm-bbbbbbbbbbbb", "");
  await assert.rejects(reapStaleVms(host.base, "/opt/fc/firecracker", host.cg, 100), VmLeakError);
  assert.deepEqual(await readdir(path.join(host.base, "firecracker")), ["bm-aaaaaaaaaaaa"]);
  assert.deepEqual(await readdir(path.join(host.base, "sockets")), ["bm-aaaaaaaaaaaa.sock"]);
  await host.cleanup();
});

test("the reaper removes dead VMs", async () => {
  const host = await fakeHost();
  await host.vm("bm-bbbbbbbbbbbb", "");
  assert.equal(await reapStaleVms(host.base, "/opt/fc/firecracker", host.cg, 100), 1);
  assert.deepEqual(await readdir(path.join(host.base, "firecracker")), []);
  assert.deepEqual(await readdir(path.join(host.base, "sockets")), []);
  await host.cleanup();
});

const sameIds = process.getuid?.() !== undefined && process.getuid() === process.getgid?.();

test("startJailedVm: a VM that cannot be killed is reported as a leak and its chroot is kept", { skip: !sameIds && "needs uid == gid to chown to itself" }, async () => {
  const host = await fakeHost();
  const jailer = path.join(host.dir, "jailer");
  // Stands in for the jailer: "starts" a VM by filling its cgroup.procs, which nothing ever empties.
  await writeFile(jailer, '#!/bin/sh\nwhile [ $# -gt 0 ]; do case $1 in --id) id=$2; shift 2;; --) break;; *) shift;; esac; done\nmkdir -p "$BM_FAKE_CG/firecracker/$id" && echo 4242 > "$BM_FAKE_CG/firecracker/$id/cgroup.procs"\n');
  await chmod(jailer, 0o755);
  const noop = path.join(host.dir, "noop-jailer");
  await writeFile(noop, "#!/bin/sh\nexit 0\n");
  await chmod(noop, 0o755);
  const file = path.join(host.dir, "blob");
  await writeFile(file, "kernel-or-rootfs");
  process.env.BM_FAKE_CG = host.cg;
  const spec = {
    firecracker: path.join(host.dir, "firecracker"), kernel: file, rootfs: file, chrootBase: host.base, cgroupRoot: host.cg,
    uid: process.getuid!(), socketGid: process.getgid!(), killWaitMs: 100,
  };
  try {
    // The vsock socket never appears, so the start fails after the VM is "running"; killing it does not work.
    await assert.rejects(startJailedVm({ ...spec, jailer }), VmLeakError);
    const left = await readdir(path.join(host.base, "firecracker"));
    assert.equal(left.length, 1, "the live VM's chroot must not be removed");
    assert.ok((await stat(path.join(host.base, "firecracker", left[0], "root", "rootfs.ext4"))).isFile());

    // A jailer that starts nothing: an ordinary failure, and everything is cleaned up.
    await assert.rejects(startJailedVm({ ...spec, jailer: noop }), (e: unknown) => e instanceof Error && !(e instanceof VmLeakError));
    assert.deepEqual(await readdir(path.join(host.base, "firecracker")), left);
  } finally {
    delete process.env.BM_FAKE_CG;
    await host.cleanup();
  }
});
