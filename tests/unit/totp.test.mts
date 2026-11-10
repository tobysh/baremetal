import test from "node:test";
import assert from "node:assert/strict";
import { createHmac } from "node:crypto";

process.env.AUTH_SECRET = "unit-test-secret-unit-test-secret-0123456789";
const totp = await import("../../src/lib/totp");

// Independent RFC 6238 SHA-1 implementation so the test does not reuse the code under test.
const b32 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
const decode = (s: string) => {
  let bits = ""; for (const c of s) bits += b32.indexOf(c).toString(2).padStart(5, "0");
  return Buffer.from(bits.match(/.{8}/g)!.map((b) => parseInt(b, 2)));
};
const code = (secret: string, step: number) => {
  const m = Buffer.alloc(8); m.writeBigUInt64BE(BigInt(step));
  const h = createHmac("sha1", decode(secret)).update(m).digest(); const o = h[19] & 15;
  return String((h.readUInt32BE(o) & 0x7fffffff) % 1_000_000).padStart(6, "0");
};
const nowStep = () => Math.floor(Date.now() / 30_000);

test("accepts the current code and returns its step", () => {
  const s = totp.newTotpSecret();
  assert.equal(totp.verifyTotp(s, code(s, nowStep())), nowStep());
  assert.equal(totp.verifyTotp(s, ` ${code(s, nowStep()).slice(0, 3)} ${code(s, nowStep()).slice(3)} `), nowStep());
});

test("allows one step of drift, not two", () => {
  const s = totp.newTotpSecret();
  assert.equal(totp.verifyTotp(s, code(s, nowStep() - 1)), nowStep() - 1);
  assert.equal(totp.verifyTotp(s, code(s, nowStep() + 1)), nowStep() + 1);
  assert.equal(totp.verifyTotp(s, code(s, nowStep() - 3)), null);
});

test("replay: a step at or before lastUsedStep is refused", () => {
  const s = totp.newTotpSecret();
  const step = totp.verifyTotp(s, code(s, nowStep()))!;
  assert.equal(totp.verifyTotp(s, code(s, nowStep()), step), null);
});

test("rejects malformed codes", () => {
  const s = totp.newTotpSecret();
  for (const bad of ["", "12345", "1234567", "abcdef", "12 34 5x"]) assert.equal(totp.verifyTotp(s, bad), null);
});

test("backup codes: format, normalisation and keyed hash", () => {
  const codes = totp.newBackupCodes(5);
  assert.equal(new Set(codes).size, 5);
  for (const c of codes) assert.match(c, /^[a-z2-9]{4}(-[a-z2-9]{4}){3}$/);
  const c = codes[0];
  assert.equal(totp.hashBackupCode(c), totp.hashBackupCode(c.toUpperCase().replace(/-/g, " ")));
  assert.match(totp.hashBackupCode(c), /^v2:/);
  assert.notEqual(totp.hashBackupCode(c), totp.legacyBackupCodeHash(c));
});

test("otpauth URL carries the secret and issuer", () => {
  const u = totp.otpauthUrl("ABCDEFGH", "a@b.c", "Baremetal");
  assert.match(u, /^otpauth:\/\/totp\/Baremetal%3Aa%40b\.c\?secret=ABCDEFGH&issuer=Baremetal/);
});
