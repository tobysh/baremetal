import { randomBytes } from "node:crypto";
import { createServer, type Server, type Socket } from "node:net";
import { connectGuest, type Vm } from "./vm";
import { encodeFrame, FrameError, FrameParser, FrameType, resizePayload, type Frame } from "./frames";

export type BrokerLimits = {
  /** Sessions alive at once (booting ones count), sized from host RAM. */
  maxSessions: number;
  /** No keystrokes or resizes for this long ends the session. */
  idleMs: number;
  /** Hard wall-clock cap from admission, boot included. */
  absoluteMs: number;
  /** Time allowed for the guest agent to report ready. */
  bootMs: number;
  /** Time allowed between connecting and sending a valid OPEN frame. */
  openMs: number;
  /** Token bucket for client input bytes. */
  inputBurst: number;
  inputPerSec: number;
  /** Total bytes of terminal output per session. */
  maxOutputBytes: number;
  /**
   * Token bucket for guest output. When it runs dry the guest socket is paused (backpressure reaches the guest agent and
   * its PTY), so a fast writer is slowed down, not killed.
   */
  outputBurst: number;
  outputPerSec: number;
  /** Output that keeps the bucket below a quarter full for this long ends the session (a sustained flood). */
  outputFloodMs: number;
};

export const DEFAULT_LIMITS: BrokerLimits = {
  maxSessions: 4,
  idleMs: 5 * 60_000,
  absoluteMs: 30 * 60_000,
  bootMs: 15_000,
  openMs: 5_000,
  inputBurst: 64 * 1024,
  inputPerSec: 16 * 1024,
  maxOutputBytes: 32 * 1024 * 1024,
  outputBurst: 512 * 1024,
  outputPerSec: 128 * 1024,
  outputFloodMs: 60_000,
};

/** Audit records carry ids and counters only, never terminal content. */
export type AuditEvent =
  | { event: "rejected"; reason: string; account?: string }
  | { event: "session_start"; id: string; account: string; bootMs: number }
  | { event: "session_end"; id: string; account?: string; reason: string; durationMs: number; bytesIn: number; bytesOut: number };

export type BrokerOptions = {
  startVm: () => Promise<Vm>;
  limits?: Partial<BrokerLimits>;
  audit?: (event: AuditEvent) => void;
};

class Bucket {
  private tokens: number;
  private last = Date.now();
  constructor(private burst: number, private perSec: number) { this.tokens = burst; }
  take(n: number): boolean {
    const now = Date.now();
    this.tokens = Math.min(this.burst, this.tokens + ((now - this.last) / 1000) * this.perSec);
    this.last = now;
    if (n > this.tokens) return false;
    this.tokens -= n;
    return true;
  }
  /** Take `n` even if that goes into debt; returns how long until the bucket is back at zero. */
  charge(n: number): number {
    this.take(0);
    this.tokens -= n;
    return this.tokens < 0 ? Math.ceil((-this.tokens / this.perSec) * 1000) : 0;
  }
  get level() { return this.tokens; }
}

const ACCOUNT = /^[A-Za-z0-9_-]{8,128}$/;
const inRange = (n: unknown, max: number): n is number => typeof n === "number" && Number.isInteger(n) && n >= 1 && n <= max;

function parseOpen(payload: Buffer): { account: string; cols: number; rows: number } | null {
  try {
    const value = JSON.parse(payload.toString("utf8")) as Record<string, unknown>;
    if (typeof value.account !== "string" || !ACCOUNT.test(value.account) || !inRange(value.cols, 500) || !inRange(value.rows, 200)) return null;
    return { account: value.account, cols: value.cols, rows: value.rows };
  } catch {
    return null;
  }
}

const errorFrame = (code: string, message: string) => encodeFrame(FrameType.Error, Buffer.from(JSON.stringify({ code, message })));

/** Reasons the learner should be told about; the others are normal ends. */
const MESSAGES: Record<string, string> = {
  busy: "No shell is free right now. Try again in a minute.",
  account_busy: "You already have a shell open.",
  bad_open: "Invalid request.",
  idle_timeout: "The shell closed after a period of inactivity.",
  time_limit: "The shell reached its time limit.",
  output_limit: "The shell produced too much output.",
  output_flood: "The shell produced output too fast for too long.",
  input_rate: "Input arrived too fast and the shell was closed.",
  boot_failed: "The shell could not start.",
  protocol: "Protocol error.",
  guest_protocol: "The shell ended unexpectedly.",
  guest_gone: "The shell ended unexpectedly.",
  vm_exited: "The shell ended unexpectedly.",
  broker_shutdown: "The shell service is restarting.",
};

export function createBroker(options: BrokerOptions) {
  const limits = { ...DEFAULT_LIMITS, ...options.limits };
  const audit = options.audit ?? ((e: AuditEvent) => console.log(JSON.stringify({ at: new Date().toISOString(), ...e })));
  const sessions = new Set<{ end: (reason: string) => Promise<void> }>();
  const accounts = new Set<string>();
  let reserved = 0; // sessions admitted (booting + live) until their VM is destroyed

  const server: Server = createServer((client) => handle(client));

  function handle(client: Socket) {
    const id = randomBytes(8).toString("hex");
    const started = Date.now();
    const clientParser = new FrameParser();
    const guestParser = new FrameParser();
    const input = new Bucket(limits.inputBurst, limits.inputPerSec);
    const output = new Bucket(limits.outputBurst, limits.outputPerSec);
    let calmAt = Date.now(); // last time the output bucket was at least a quarter full
    // The guest is read only while neither the web side nor the output bucket asks to wait.
    let clientFull = false;
    let rateWait: NodeJS.Timeout | undefined;
    const flowGuest = () => { if (guest && !ended()) { if (clientFull || rateWait) guest.pause(); else guest.resume(); } };
    let state: "waiting" | "booting" | "live" | "ended" = "waiting";
    let account: string | undefined;
    let vm: Vm | undefined;
    let vmStart: Promise<Vm> | undefined;
    let guest: Socket | undefined;
    let admitted = false;
    let bytesIn = 0;
    let bytesOut = 0;
    let idleTimer: NodeJS.Timeout | undefined;
    let absoluteTimer: NodeJS.Timeout | undefined;
    let bootTimer: NodeJS.Timeout | undefined;
    const openTimer = setTimeout(() => void end("bad_open"), limits.openMs);
    client.setNoDelay(true);

    const ended = () => state === "ended"; // a function call, so TypeScript does not narrow `state` across awaits
    const session = { end: (reason: string) => end(reason) };
    let ending: Promise<void> | undefined;

    function end(reason: string): Promise<void> {
      return (ending ??= (async () => {
        const previous = state;
        state = "ended";
        for (const t of [openTimer, idleTimer, absoluteTimer, bootTimer, rateWait]) clearTimeout(t);
        sessions.delete(session);
        if (client.writable && MESSAGES[reason]) client.write(errorFrame(reason, MESSAGES[reason]));
        client.end();
        guest?.destroy();
        try {
          // A VM that was still starting when we ended must be waited for, or it would outlive the session.
          const running = vm ?? (await vmStart?.catch(() => undefined));
          await running?.destroy();
        } finally {
          // Capacity and the per-account lock are released only once the VM is really gone.
          if (admitted) {
            reserved--;
            if (account) accounts.delete(account);
          }
          if (previous === "waiting" && !admitted) audit({ event: "rejected", reason, account });
          else audit({ event: "session_end", id, account, reason, durationMs: Date.now() - started, bytesIn, bytesOut });
          setTimeout(() => client.destroy(), 2_000).unref(); // let the final frame flush; a client that never reads is cut off
        }
      })());
    }

    const touch = () => {
      clearTimeout(idleTimer);
      idleTimer = setTimeout(() => void end("idle_timeout"), limits.idleMs);
    };

    client.on("error", () => void end("client_gone"));
    client.on("close", () => void end("client_gone"));
    client.on("data", (chunk) => {
      if (state === "ended") return;
      let frames: Frame[];
      try {
        frames = clientParser.push(chunk);
      } catch (error) {
        if (error instanceof FrameError) return void end(state === "waiting" ? "bad_open" : "protocol");
        throw error;
      }
      for (const frame of frames) {
        if (state === "waiting") void open(frame);
        else if (state === "live") onClientFrame(frame);
        else return void end("protocol"); // anything while booting, or after the end, is a violation
        if (ended()) return;
      }
    });

    async function open(frame: Frame) {
      clearTimeout(openTimer);
      const request = frame.type === FrameType.Open ? parseOpen(frame.payload) : null;
      if (!request) return void end("bad_open");
      account = request.account;
      if (accounts.has(account)) return void end("account_busy"); // the more specific answer wins over "busy"
      if (reserved >= limits.maxSessions) return void end("busy");
      accounts.add(account);
      reserved++;
      admitted = true;
      state = "booting";
      sessions.add(session);
      absoluteTimer = setTimeout(() => void end("time_limit"), limits.absoluteMs);
      bootTimer = setTimeout(() => void end("boot_failed"), limits.bootMs);
      try {
        vmStart = options.startVm();
        vm = await vmStart;
        if (ended()) return; // end() is waiting on vmStart and destroys it
        void vm.exited.then(() => end("vm_exited"));
        const connected = await connectGuest(vm.socketPath, Date.now() + limits.bootMs, () => ended());
        guest = connected.socket;
        if (ended()) return void guest.destroy();
        guest.setNoDelay(true);
        guest.on("error", () => void end("guest_gone"));
        guest.on("close", () => void end("guest_gone"));
        guest.on("data", (chunk) => onGuestData(chunk, request));
        if (connected.rest.length) onGuestData(connected.rest, request);
      } catch {
        void end("boot_failed");
      }
    }

    function onGuestData(chunk: Buffer, request: { cols: number; rows: number }) {
      if (ended()) return;
      let frames: Frame[];
      try {
        frames = guestParser.push(chunk);
      } catch {
        return void end("guest_protocol");
      }
      for (const frame of frames) {
        if (ended()) return;
        if (frame.type === FrameType.Ready && state === "booting") {
          clearTimeout(bootTimer);
          guest!.write(encodeFrame(FrameType.Resize, resizePayload(request.cols, request.rows)));
          state = "live";
          touch();
          client.write(encodeFrame(FrameType.Opened));
          audit({ event: "session_start", id, account: account!, bootMs: Date.now() - started });
        } else if (frame.type === FrameType.Data && state === "live") {
          bytesOut += frame.payload.length;
          if (bytesOut > limits.maxOutputBytes) return void end("output_limit");
          const waitMs = output.charge(frame.payload.length);
          if (output.level >= limits.outputBurst / 4) calmAt = Date.now();
          else if (Date.now() - calmAt > limits.outputFloodMs) return void end("output_flood");
          if (!client.write(encodeFrame(FrameType.Data, frame.payload)) && !clientFull) {
            // Backpressure: stop reading the guest until the web side catches up.
            clientFull = true;
            client.once("drain", () => { clientFull = false; flowGuest(); });
          }
          if (waitMs > 0 && !rateWait) {
            // Over the output rate: stop reading the guest until the bucket has refilled. Frames already parsed from
            // this chunk are still delivered, so the overshoot is at most one socket read.
            rateWait = setTimeout(() => { rateWait = undefined; flowGuest(); }, waitMs);
          }
          flowGuest();
        } else if (frame.type === FrameType.Exit && state === "live" && frame.payload.length === 4) {
          client.write(encodeFrame(FrameType.Exit, frame.payload));
          return void end("shell_exit");
        } else {
          return void end("guest_protocol");
        }
      }
    }

    function onClientFrame(frame: Frame) {
      if (frame.type === FrameType.Data) {
        bytesIn += frame.payload.length;
        if (!input.take(frame.payload.length)) return void end("input_rate");
        touch();
        if (!guest!.write(encodeFrame(FrameType.Data, frame.payload))) {
          client.pause();
          guest!.once("drain", () => client.resume());
        }
      } else if (frame.type === FrameType.Resize && frame.payload.length === 4 && inRange(frame.payload.readUInt16BE(0), 500) && inRange(frame.payload.readUInt16BE(2), 200)) {
        touch();
        guest!.write(encodeFrame(FrameType.Resize, frame.payload));
      } else {
        void end("protocol");
      }
    }
  }

  return {
    server,
    stats: () => ({ reserved, sessions: sessions.size, accounts: accounts.size }),
    async shutdown() {
      server.close();
      await Promise.all([...sessions].map((s) => s.end("broker_shutdown")));
    },
  };
}
