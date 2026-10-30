import { connect } from "node:net";
import { MAX_LINE } from "./launcher";
import type { Vm } from "./vm";

/**
 * Broker side of the launcher protocol (see launcher.ts): each VM is one connection to the launcher's Unix socket, held
 * open for the VM's life. Closing it is the destroy request, so a crashed broker cannot leak VMs.
 */
export function launcherVmStarter(launcherSocket: string, timeoutMs = 30_000): () => Promise<Vm> {
  return () =>
    new Promise<Vm>((resolve, reject) => {
      const conn = connect(launcherSocket);
      let buffered = "";
      let settled = false;
      let resolveExited!: () => void;
      const exited = new Promise<void>((r) => (resolveExited = r));
      const closed = new Promise<void>((r) => conn.once("close", () => r()));
      const timer = setTimeout(() => fail(new Error("launcher did not answer")), timeoutMs);

      function fail(error: Error) {
        clearTimeout(timer);
        conn.destroy();
        if (!settled) { settled = true; reject(error); }
      }
      let destroying: Promise<void> | undefined;
      const destroy = () => (destroying ??= (async () => {
        if (conn.writable) conn.end(`${JSON.stringify({ op: "destroy" })}\n`);
        // The launcher closes only after the VM is gone; a launcher that hangs is cut off (its own lease rules still apply).
        const hung = setTimeout(() => conn.destroy(), 20_000);
        await closed;
        clearTimeout(hung);
      })());

      conn.on("error", (error) => fail(error));
      conn.on("close", () => {
        resolveExited();
        fail(new Error("launcher closed the connection"));
      });
      conn.once("connect", () => conn.write(`${JSON.stringify({ op: "create" })}\n`));
      conn.on("data", (chunk) => {
        buffered += chunk.toString("utf8");
        let nl: number;
        while ((nl = buffered.indexOf("\n")) >= 0) {
          const text = buffered.slice(0, nl);
          buffered = buffered.slice(nl + 1);
          let message: { ok?: unknown; socket?: unknown; code?: unknown; event?: unknown };
          try {
            message = JSON.parse(text);
          } catch {
            return fail(new Error("launcher sent garbage"));
          }
          if (!settled && message.ok === true && typeof message.socket === "string" && message.socket.startsWith("/") && Buffer.byteLength(message.socket) <= 107) {
            settled = true;
            clearTimeout(timer);
            resolve({ socketPath: message.socket, exited, destroy });
          } else if (!settled) {
            return fail(new Error(`launcher refused: ${typeof message.code === "string" ? message.code : "unknown"}`));
          } else if (message.event === "exited") {
            resolveExited();
          } else {
            conn.destroy(); // the VM's lease ends with the connection; the broker sees `exited`
          }
        }
        if (buffered.length > MAX_LINE * 4) conn.destroy();
      });
    });
}
