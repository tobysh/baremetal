import test from "node:test";
import assert from "node:assert/strict";

process.env.AUTH_SECRET = "unit-test-secret-unit-test-secret-0123456789";
const c = await import("../../src/lib/crypto");

test("encrypt/decrypt round trip with fresh IVs", () => {
  const a = c.encrypt("héllo secret"), b = c.encrypt("héllo secret");
  assert.notEqual(a, b);
  assert.equal(c.decrypt(a), "héllo secret");
  assert.equal(a.split(".").length, 3);
});

test("tampering with any part fails authentication", () => {
  const [iv, tag, ct] = c.encrypt("payload").split(".");
  const flip = (s: string) => (s[0] === "A" ? "B" : "A") + s.slice(1);
  for (const bad of [[flip(iv), tag, ct], [iv, flip(tag), ct], [iv, tag, flip(ct)]]) assert.throws(() => c.decrypt(bad.join(".")));
  assert.throws(() => c.decrypt("garbage"));
});

test("hmac is purpose-separated and deterministic", () => {
  assert.equal(c.hmac("a", "x"), c.hmac("a", "x"));
  assert.notEqual(c.hmac("a", "x"), c.hmac("b", "x"));
  assert.notEqual(c.hmac("a", "x"), c.hmac("a", "y"));
});

test("safeEqual handles length mismatch", () => {
  assert.equal(c.safeEqual("abc", "abc"), true);
  assert.equal(c.safeEqual("abc", "abd"), false);
  assert.equal(c.safeEqual("abc", "abcd"), false);
});

test("tokens and hashes", () => {
  assert.notEqual(c.randomToken(), c.randomToken());
  assert.match(c.sha256("abc"), /^ba7816bf8f01cfea/);
});

test("a short AUTH_SECRET is refused", async () => {
  const saved = process.env.AUTH_SECRET;
  process.env.AUTH_SECRET = "short";
  try { assert.throws(() => c.hmac("a", "x"), /AUTH_SECRET/); } finally { process.env.AUTH_SECRET = saved; }
});
