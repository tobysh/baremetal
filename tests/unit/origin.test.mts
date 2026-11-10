import test from "node:test";
import assert from "node:assert/strict";
import { sameSiteOrigin } from "../../src/lib/origin";

const site = "https://baremetal.example";
test("matches scheme, host and port", () => {
  assert.equal(sameSiteOrigin("https://baremetal.example", site), true);
  assert.equal(sameSiteOrigin("http://baremetal.example", site), false);
  assert.equal(sameSiteOrigin("https://baremetal.example:8443", site), false);
  assert.equal(sameSiteOrigin("https://evil.example", site), false);
  assert.equal(sameSiteOrigin("null", site), false);
});
