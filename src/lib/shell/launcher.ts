import { createServer, type Server, type Socket } from "node:net";
import type { Vm } from "./vm";

/**
 * The privileged half of the shell service. Starting the jailer needs root, so a tiny launcher runs as root and does
 * only this: create a VM, destroy it, and reap leftovers at startup. It parses nothing from the guest and holds no
 * learner data; the unprivileged broker (broker.ts) does the rest. See docs/interactive-shell-isolation.md.
 *
 * Protocol, one VM per connection, newline-terminated JSON lines of at most MAX_LINE bytes:
 *   broker -> launcher  {"op":"create"}                      once, first
 *   launcher -> broker  {"ok":true,"socket":"/abs/path"}     the VM's vsock socket, reachable by the broker's group
 *                   or  {"ok":false,"code":"busy"|"start_failed"}  then close
 *   broker -> launcher  {"op":"destroy"}                     or simply close the connection
 *   launcher -> broker  {"event":"exited"}                   the VM is gone and its files removed; then close
 * The connection is the VM's lease: when it drops for any reason (broker crash included) the VM is destroyed, and
 * "exited" is sent only after destroy() has finished. Requests carry no parameters, so the kernel, rootfs, chroot base,
 * uid and limits can only come from the launcher's own configuration.
 */
export const MAX_LINE = 128;

export type LauncherLimits = {
  /** VMs alive at once, including ones starting or being destroyed. The hard cap, whatever the broker believes. */
  maxVms: number;
  /** A VM is destroyed after this long even if the broker still holds it (defence in depth for a hung broker). */
  maxLifetimeMs: number;
  /** Time allowed between connecting and sending "create". */
  requestMs: number;
};

export const DEFAULT_LAUNCHER_LIMITS: LauncherLimits = { maxVms: 4, maxLifetimeMs: 35 * 60_000, requestMs: 2_000 };

export type LauncherEvent =
  | { event: "vm_start"; slot: number }
  | { event: "vm_end"; slot: number; reason: string; lifetimeMs: number }
  | { event: "refused"; reason: string }
  | { event: "destroy_failed"; slot: number; error: string };

export type LauncherOptions = {
  /** Start a VM in `slot` (0 .. maxVms-1). A slot is reused only after its previous VM was destroyed. */
  startVm: (slot: number) => Promise<Vm>;
  limits?: Partial<LauncherLimits>;
  log?: (event: LauncherEvent) => void;
};

const line = (value: object) => `${JSON.stringify(value)}\n`;
/** Exact match only: no extra keys, no parameters. */
const isOp = (text: string, op: string) => {
  try {
    const value = JSON.parse(text) as unknown;
    return typeof value === "object" && value !== null && !Array.isArray(value) && Object.keys(value).length === 1 && (value as { op?: unknown }).op === op;
  } catch {
    return false;
  }
};

export function createLauncher(options: LauncherOptions) {
  const limits = { ...DEFAULT_LAUNCHER_LIMITS, ...options.limits };
  const log = options.log ?? ((e: LauncherEvent) => console.log(JSON.stringify({ at: new Date().toISOString(), ...e })));
  const free = Array.from({ length: limits.maxVms }, (_, i) => i);
  const leases = new Set<(reason: string) => Promise<void>>();

  // allowHalfOpen: a broker that half-closes still gets the final "exited" line.
  const server: Server = createServer({ allowHalfOpen: true }, (conn) => handle(conn));
  // Only the broker can connect (socket permissions), but cap descriptors anyway: idle connections cost no slot.
  server.maxConnections = limits.maxVms * 2 + 4;

  function handle(conn: Socket) {
    let buffered = "";
    let state: "new" | "starting" | "running" | "closing" = "new";
    let slot: number | undefined;
    let vm: Vm | undefined;
    let start: Promise<Vm> | undefined;
    let startedAt = 0;
    let lifetimeTimer: NodeJS.Timeout | undefined;
    const requestTimer = setTimeout(() => void close("no_request"), limits.requestMs);
    let closing: Promise<void> | undefined;
    const isClosing = () => state === "closing"; // a call, so TypeScript does not narrow `state` across awaits

    function close(reason: string, reply?: object): Promise<void> {
      return (closing ??= (async () => {
        state = "closing";
        clearTimeout(requestTimer);
        clearTimeout(lifetimeTimer);
        leases.delete(close);
        let destroyedOk = true;
        try {
          // A VM still starting must be waited for, or it would outlive its lease.
          const running = vm ?? (await start?.catch(() => undefined));
          await running?.destroy();
        } catch (error) {
          destroyedOk = false;
          log({ event: "destroy_failed", slot: slot!, error: String(error) });
        } finally {
          if (slot !== undefined) {
            // A VM that may still be running keeps its slot, and so its uid, out of use until the next restart reaps it.
            if (destroyedOk) free.push(slot);
            log({ event: "vm_end", slot, reason, lifetimeMs: startedAt ? Date.now() - startedAt : 0 });
          }
          const final = reply ?? (slot !== undefined ? { event: "exited" } : undefined);
          if (conn.writable) { if (final) conn.end(line(final)); else conn.end(); }
          setTimeout(() => conn.destroy(), 1_000).unref();
        }
      })());
    }

    conn.on("error", () => void close("connection_lost"));
    conn.on("close", () => void close("connection_lost"));
    conn.on("end", () => void close("released"));
    conn.on("data", (chunk) => {
      if (state === "closing") return;
      buffered += chunk.toString("latin1");
      if (buffered.length > MAX_LINE * 2) return void close("bad_request");
      let nl: number;
      while ((nl = buffered.indexOf("\n")) >= 0) {
        const text = buffered.slice(0, nl);
        buffered = buffered.slice(nl + 1);
        if (text.length > MAX_LINE) return void close("bad_request");
        if (state === "new" && isOp(text, "create")) void create();
        else if (state === "running" && isOp(text, "destroy")) return void close("released");
        else return void close("bad_request"); // anything else, including a request while starting, ends the lease
      }
      if (buffered.length > MAX_LINE) void close("bad_request");
    });

    async function create() {
      clearTimeout(requestTimer);
      slot = free.shift();
      if (slot === undefined) {
        log({ event: "refused", reason: "busy" });
        return void close("busy", { ok: false, code: "busy" });
      }
      state = "starting";
      leases.add(close);
      startedAt = Date.now();
      lifetimeTimer = setTimeout(() => void close("lifetime"), limits.maxLifetimeMs);
      try {
        start = options.startVm(slot);
        vm = await start;
      } catch {
        return void close("start_failed", { ok: false, code: "start_failed" });
      }
      if (isClosing()) return; // close() is waiting on `start` and destroys the VM
      state = "running";
      log({ event: "vm_start", slot });
      void vm.exited.then(() => close("vm_exited"));
      conn.write(line({ ok: true, socket: vm.socketPath }));
    }
  }

  return {
    server,
    stats: () => ({ active: limits.maxVms - free.length, leases: leases.size }),
    /** Stop accepting and destroy every VM. Run before exit so no chroot or cgroup is left behind. */
    async shutdown() {
      server.close();
      await Promise.all([...leases].map((close) => close("launcher_shutdown")));
    },
  };
}
