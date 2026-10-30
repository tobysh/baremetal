/**
 * Length-prefixed frames shared by the guest agent (scripts/shell/agent.c), the broker and its web-side client:
 * [type u8][length u32 big-endian][payload]. Both ends of every hop are untrusted by the other, so the parser
 * enforces the payload bound and never buffers more than one frame.
 */
export const MAX_PAYLOAD = 16 * 1024;

export const FrameType = {
  Data: 0,
  Resize: 1, // u16 cols, u16 rows
  Ready: 2, // guest -> broker: the shell is running
  Exit: 3, // i32 status
  Open: 0x10, // web -> broker, JSON
  Opened: 0x11, // broker -> web
  Error: 0x12, // broker -> web, JSON { code, message }
} as const;

export type Frame = { type: number; payload: Buffer };

export class FrameError extends Error {}

export function encodeFrame(type: number, payload: Uint8Array = new Uint8Array(0)): Buffer {
  if (payload.length > MAX_PAYLOAD) throw new FrameError("frame payload too large");
  const out = Buffer.allocUnsafe(5 + payload.length);
  out[0] = type;
  out.writeUInt32BE(payload.length, 1);
  out.set(payload, 5);
  return out;
}

export const resizePayload = (cols: number, rows: number) => {
  const b = Buffer.allocUnsafe(4);
  b.writeUInt16BE(cols, 0);
  b.writeUInt16BE(rows, 2);
  return b;
};

export class FrameParser {
  private pending: Buffer = Buffer.alloc(0);

  /** Feed bytes, get complete frames. Throws FrameError on an oversize length; the caller must drop the connection. */
  push(chunk: Buffer): Frame[] {
    this.pending = this.pending.length ? Buffer.concat([this.pending, chunk]) : chunk;
    const frames: Frame[] = [];
    for (;;) {
      if (this.pending.length < 5) break;
      const length = this.pending.readUInt32BE(1);
      if (length > MAX_PAYLOAD) throw new FrameError("frame length exceeds limit");
      if (this.pending.length < 5 + length) break;
      frames.push({ type: this.pending[0], payload: Buffer.from(this.pending.subarray(5, 5 + length)) });
      this.pending = this.pending.subarray(5 + length);
    }
    // A partial frame is at most 5 + MAX_PAYLOAD bytes, so memory per connection is bounded.
    if (this.pending.length > 5 + MAX_PAYLOAD) throw new FrameError("buffer limit");
    if (!this.pending.length) this.pending = Buffer.alloc(0);
    return frames;
  }
}
