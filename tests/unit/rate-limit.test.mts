import test from "node:test";
import assert from "node:assert/strict";

const rl = await import("../../src/lib/rate-limit");
const h = (xff?: string) => new Headers(xff === undefined ? {} : { "x-forwarded-for": xff });

test("clientIp reads the entry our proxy added, never the client-controlled left end", () => {
  delete process.env.TRUSTED_PROXY_HOPS;
  assert.equal(rl.clientIp(h("6.6.6.6, 203.0.113.9")), "203.0.113.9");
  process.env.TRUSTED_PROXY_HOPS = "2";
  assert.equal(rl.clientIp(h("6.6.6.6, 203.0.113.9, 10.0.0.1")), "203.0.113.9");
  delete process.env.TRUSTED_PROXY_HOPS;
});

test("a chain shorter than the hop count falls into the unknown bucket", () => {
  process.env.TRUSTED_PROXY_HOPS = "2";
  assert.equal(rl.clientIp(h("6.6.6.6")), rl.UNKNOWN_IP);
  assert.equal(rl.clientIp(h()), rl.UNKNOWN_IP);
  process.env.TRUSTED_PROXY_HOPS = "0"; // clamps to 1
  assert.equal(rl.clientIp(h("1.1.1.1")), "1.1.1.1");
  process.env.TRUSTED_PROXY_HOPS = "junk";
  assert.equal(rl.clientIp(h("1.1.1.1")), "1.1.1.1");
  delete process.env.TRUSTED_PROXY_HOPS;
});

test("ipLimit quarters the unknown bucket only", () => {
  assert.equal(rl.ipLimit(rl.UNKNOWN_IP, 40), 10);
  assert.equal(rl.ipLimit(rl.UNKNOWN_IP, 2), 1);
  assert.equal(rl.ipLimit("1.2.3.4", 40), 40);
});
