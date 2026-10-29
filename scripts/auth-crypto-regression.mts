// Run: npx tsx --conditions=react-server --env-file=.env.local scripts/auth-crypto-regression.mts
import assert from "node:assert/strict";
import { createHash, createHmac } from "node:crypto";
import { hashBackupCode, legacyBackupCodeHash, newBackupCodes, verifyTotp } from "../src/lib/totp";

const codes = newBackupCodes(100);
assert.equal(new Set(codes).size, codes.length);
for (const code of codes) {
  assert.match(code, /^[a-z2-9]{4}(?:-[a-z2-9]{4}){3}$/);
  assert.equal(hashBackupCode(code), hashBackupCode(code.toUpperCase().replaceAll("-", " ")));
  assert.notEqual(hashBackupCode(code), legacyBackupCodeHash(code));
}
const old = "k3f9-x2mq";
assert.equal(legacyBackupCodeHash(old), createHash("sha256").update("backup:k3f9x2mq").digest("hex"));
assert.match(hashBackupCode(old), /^v2:/);

// A known secret with a code in the current time step: accepted once, rejected on replay.
const secret = "JBSWY3DPEHPK3PXP";
const step = Math.floor(Date.now() / 30000);
const counter = Buffer.alloc(8);
counter.writeBigUInt64BE(BigInt(step));
const digest = createHmac("sha1", Buffer.from("48656c6c6f21deadbeef", "hex")).update(counter).digest();
const o = digest[digest.length - 1] & 15;
const value = (((digest[o] & 127) << 24) | (digest[o + 1] << 16) | (digest[o + 2] << 8) | digest[o + 3]) % 1_000_000;
const token = String(value).padStart(6, "0");
assert.equal(verifyTotp(secret, token), step);
assert.equal(verifyTotp(secret, token, step), null);
console.log("Auth crypto regressions passed.");
