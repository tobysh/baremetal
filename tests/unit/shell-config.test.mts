import test from "node:test";
import assert from "node:assert/strict";
import { shellBrokerSocket } from "../../src/lib/shell/config";

test("a broker socket alone cannot enable unverified shells in production", () => {
  for (const verification of [undefined, "", "0", "true"]) {
    assert.equal(shellBrokerSocket({ NODE_ENV: "production", SHELL_BROKER_SOCKET: "/run/shell.sock", SHELL_HOST_VERIFIED: verification }), undefined);
  }
});

test("verified production hosts still need a configured broker socket", () => {
  assert.equal(shellBrokerSocket({ NODE_ENV: "production", SHELL_HOST_VERIFIED: "1" }), undefined);
  assert.equal(shellBrokerSocket({ NODE_ENV: "production", SHELL_HOST_VERIFIED: "1", SHELL_BROKER_SOCKET: "/run/shell.sock" }), "/run/shell.sock");
});

test("local shell development remains available without production attestation", () => {
  assert.equal(shellBrokerSocket({ NODE_ENV: "development", SHELL_BROKER_SOCKET: "/tmp/shell.sock" }), "/tmp/shell.sock");
  assert.equal(shellBrokerSocket({ NODE_ENV: "test", SHELL_BROKER_SOCKET: " " }), undefined);
});
