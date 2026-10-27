import "server-only";
import { createHmac, randomBytes } from "node:crypto";
import { safeEqual, sha256 } from "./crypto";

// RFC 6238 TOTP (SHA-1, 6 digits, 30s) — what Google Authenticator, 1Password,
// Authy etc. expect.

const B32 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
const STEP = 30;

export function base32Encode(buf: Buffer): string {
  let bits = 0;
  let value = 0;
  let out = "";
  for (const byte of buf) {
    value = (value << 8) | byte;
    bits += 8;
    while (bits >= 5) {
      out += B32[(value >>> (bits - 5)) & 31];
      bits -= 5;
    }
  }
  if (bits > 0) out += B32[(value << (5 - bits)) & 31];
  return out;
}

function base32Decode(s: string): Buffer {
  const clean = s.replace(/=+$/, "").toUpperCase();
  let bits = 0;
  let value = 0;
  const out: number[] = [];
  for (const ch of clean) {
    const idx = B32.indexOf(ch);
    if (idx < 0) continue;
    value = (value << 5) | idx;
    bits += 5;
    if (bits >= 8) {
      out.push((value >>> (bits - 8)) & 255);
      bits -= 8;
    }
  }
  return Buffer.from(out);
}

export const newTotpSecret = () => base32Encode(randomBytes(20));

function hotp(secret: string, counter: number): string {
  const msg = Buffer.alloc(8);
  msg.writeBigUInt64BE(BigInt(counter));
  const h = createHmac("sha1", base32Decode(secret)).update(msg).digest();
  const offset = h[h.length - 1] & 0xf;
  const code = ((h[offset] & 0x7f) << 24) | (h[offset + 1] << 16) | (h[offset + 2] << 8) | h[offset + 3];
  return String(code % 1_000_000).padStart(6, "0");
}

/**
 * Checks a code within ±1 step of clock drift. Returns the matched time step so
 * the caller can store it and reject replays of the same code (pass
 * lastUsedStep), or null if the code is wrong.
 */
export function verifyTotp(secret: string, code: string, lastUsedStep = -1): number | null {
  const clean = code.replace(/\s+/g, "");
  if (!/^\d{6}$/.test(clean)) return null;
  const now = Math.floor(Date.now() / 1000 / STEP);
  for (const step of [now - 1, now, now + 1]) {
    if (step <= lastUsedStep) continue;
    if (safeEqual(hotp(secret, step), clean)) return step;
  }
  return null;
}

export function otpauthUrl(secret: string, account: string, issuer: string) {
  const label = encodeURIComponent(`${issuer}:${account}`);
  return `otpauth://totp/${label}?secret=${secret}&issuer=${encodeURIComponent(issuer)}&algorithm=SHA1&digits=6&period=${STEP}`;
}

// Backup codes: 10 single-use codes like "k3f9-x2mq". 40 bits of entropy each,
// stored as SHA-256 hashes.
const CODE_ALPHABET = "abcdefghjkmnpqrstuvwxyz23456789";
export function newBackupCodes(n = 10): string[] {
  return Array.from({ length: n }, () => {
    const bytes = randomBytes(8);
    const chars = Array.from(bytes, (b) => CODE_ALPHABET[b % CODE_ALPHABET.length]).join("");
    return `${chars.slice(0, 4)}-${chars.slice(4, 8)}`;
  });
}
export const normalizeBackupCode = (code: string) => code.toLowerCase().replace(/[^a-z0-9]/g, "");
export const hashBackupCode = (code: string) => sha256(`backup:${normalizeBackupCode(code)}`);
