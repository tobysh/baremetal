import "server-only";
import { connect, type Socket } from "node:net";
import { encodeFrame, FrameError, FrameParser, FrameType, MAX_PAYLOAD, resizePayload, type Frame } from "./frames";

/** The broker refused or ended the session. `code` is one of the broker's reason codes. */
export class BrokerError extends Error {
  constructor(public code: string, message: string) {
    super(message);
  }
}

export type BrokerHandlers = {
  /** Terminal output from the guest. Never log it. */
  onData: (chunk: Buffer) => void;
  onExit: (status: number) => void;
  /** The broker ended the session with a reason (idle_timeout, time_limit, ...). */
  onError: (error: BrokerError) => void;
  /** The connection is gone, for any reason. Called exactly once. */
  onClose: () => void;
};

export type BrokerConnection = {
  write: (bytes: Uint8Array) => void;
  resize: (cols: number, rows: number) => void;
  pause: () => void;
  resume: () => void;
  close: () => void;
  setHandlers: (handlers: BrokerHandlers) => void;
};


/**
 * Connects to the shell broker's Unix socket and opens a session. `account` must already be an opaque keyed
 * hash; the broker never receives an email or username. Resolves once the broker reports the shell is running.
 * Frames that arrive before `setHandlers` is called are held and replayed in order.
 */
export function openBroker(options: { socketPath: string; account: string; cols: number; rows: number; signal?: AbortSignal; timeoutMs?: number }): Promise<BrokerConnection> {
  return new Promise((resolve, reject) => {
    const socket: Socket = connect(options.socketPath);
    const parser = new FrameParser();
    let handlers: BrokerHandlers | undefined;
    const held: Array<() => void> = [];
    let opened = false;
    let settled = false;
    let closed = false;

    const emit = (fn: () => void) => (handlers ? fn() : held.push(fn));
    const fail = (error: Error) => {
      if (settled) return;
      settled = true;
      clearTimeout(timer);
      socket.destroy();
      reject(error);
    };
    const timer = setTimeout(() => fail(new BrokerError("boot_failed", "The shell could not start.")), options.timeoutMs ?? 25_000);
    const onAbort = () => fail(new BrokerError("aborted", "Cancelled."));
    if (options.signal?.aborted) return onAbort();
    options.signal?.addEventListener("abort", onAbort, { once: true });

    const connection: BrokerConnection = {
      write(bytes) {
        if (closed) return;
        for (let i = 0; i < bytes.length; i += MAX_PAYLOAD) socket.write(encodeFrame(FrameType.Data, bytes.subarray(i, i + MAX_PAYLOAD)));
      },
      resize(cols, rows) {
        if (!closed) socket.write(encodeFrame(FrameType.Resize, resizePayload(cols, rows)));
      },
      pause: () => socket.pause(),
      resume: () => socket.resume(),
      close: () => socket.destroy(),
      setHandlers(next) {
        handlers = next;
        for (const fn of held.splice(0)) fn();
      },
    };

    socket.setNoDelay(true);
    socket.on("connect", () => socket.write(encodeFrame(FrameType.Open, Buffer.from(JSON.stringify({ account: options.account, cols: options.cols, rows: options.rows })))));
    socket.on("data", (chunk) => {
      let frames: Frame[];
      try {
        frames = parser.push(chunk);
      } catch (error) {
        if (!(error instanceof FrameError)) throw error;
        if (!settled) return fail(new BrokerError("protocol", "Protocol error."));
        emit(() => handlers!.onError(new BrokerError("protocol", "Protocol error.")));
        return void socket.destroy();
      }
      for (const frame of frames) {
        if (frame.type === FrameType.Opened && !settled) {
          settled = opened = true;
          clearTimeout(timer);
          options.signal?.removeEventListener("abort", onAbort);
          resolve(connection);
        } else if (frame.type === FrameType.Error) {
          let code = "protocol";
          let message = "The shell ended unexpectedly.";
          try {
            const body = JSON.parse(frame.payload.toString("utf8")) as { code?: unknown; message?: unknown };
            if (typeof body.code === "string") code = body.code.slice(0, 40);
            if (typeof body.message === "string") message = body.message.slice(0, 200);
          } catch {}
          const error = new BrokerError(code, message);
          if (!opened) return fail(error);
          emit(() => handlers!.onError(error));
        } else if (frame.type === FrameType.Data && opened) {
          emit(() => handlers!.onData(frame.payload));
        } else if (frame.type === FrameType.Exit && opened && frame.payload.length === 4) {
          const status = frame.payload.readInt32BE(0);
          emit(() => handlers!.onExit(status));
        } else {
          if (!opened) return fail(new BrokerError("protocol", "Protocol error."));
          emit(() => handlers!.onError(new BrokerError("protocol", "Protocol error.")));
          return void socket.destroy();
        }
      }
    });
    socket.on("error", () => {
      if (!settled) fail(new BrokerError("unavailable", "The shell service is not available."));
    });
    socket.on("close", () => {
      closed = true;
      if (!settled) return fail(new BrokerError("unavailable", "The shell service is not available."));
      options.signal?.removeEventListener("abort", onAbort);
      emit(() => handlers!.onClose());
    });
  });
}
