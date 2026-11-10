import test, { afterEach } from "node:test";
import assert from "node:assert/strict";

const { register, productionConfigProblems } = await import("../../src/instrumentation");
const rl = await import("../../src/lib/rate-limit");
const env = process.env as Record<string, string | undefined>;
const saved = { ...env };
afterEach(() => {
  for (const k of Object.keys(env)) if (!(k in saved)) delete env[k];
  Object.assign(env, saved);
});

const goodProd = {
  NODE_ENV: "production", NEXT_RUNTIME: "nodejs", NEXT_PUBLIC_SITE_URL: "https://example.test", LEGAL_ENTITY_NAME: "x", LEGAL_ADDRESS: "x",
  LEGAL_CONTACT_EMAIL: "a@example.test", SMTP_URL: "smtp://x", EMAIL_FROM: "a@example.test", MONGODB_URI: "mongodb://x", AUTH_SECRET: "s".repeat(32),
};
const setEnv = (e: Record<string, string | undefined>) => { for (const [k, v] of Object.entries(e)) { if (v === undefined) delete env[k]; else env[k] = v; } };

test("productionConfigProblems is empty for a complete config and lists each missing setting", () => {
  assert.deepEqual(productionConfigProblems(goodProd as NodeJS.ProcessEnv), []);
  assert.ok(productionConfigProblems({ ...goodProd, AUTH_SECRET: "short" } as NodeJS.ProcessEnv).some((p) => p.includes("AUTH_SECRET")));
  assert.ok(productionConfigProblems({ ...goodProd, NEXT_PUBLIC_SITE_URL: "http://example.test" } as NodeJS.ProcessEnv).some((p) => p.includes("https")));
  assert.ok(productionConfigProblems({ ...goodProd, SMTP_URL: " " } as NodeJS.ProcessEnv).some((p) => p.includes("SMTP_URL")));
});

test("register() refuses production without a valid TRUSTED_PROXY_HOPS", async () => {
  for (const hops of [undefined, "", "0", "-1", "1.5", "abc", "01", " 1", "1e3", "9".repeat(20)]) {
    setEnv({ ...goodProd, TRUSTED_PROXY_HOPS: hops });
    await assert.rejects(register(), /TRUSTED_PROXY_HOPS is not set/, `hops=${JSON.stringify(hops)}`);
  }
});

test("a valid TRUSTED_PROXY_HOPS passes that check and production then validates the rest of the config", async () => {
  setEnv({ ...goodProd, TRUSTED_PROXY_HOPS: "2", AUTH_SECRET: "short", SMTP_URL: undefined });
  await assert.rejects(register(), (e: Error) => /Production configuration is incomplete/.test(e.message) && /AUTH_SECRET/.test(e.message) && !/TRUSTED_PROXY_HOPS/.test(e.message));
});

test("outside production an unset value is fine but a malformed one is still rejected", async () => {
  setEnv({ NODE_ENV: "development", NEXT_RUNTIME: "nodejs", TRUSTED_PROXY_HOPS: "0" });
  await assert.rejects(register(), /TRUSTED_PROXY_HOPS is not set/);
  setEnv({ NODE_ENV: "development", NEXT_RUNTIME: "nodejs", TRUSTED_PROXY_HOPS: "x" });
  await assert.rejects(register(), /TRUSTED_PROXY_HOPS is not set/);
});

test("register() does nothing outside the node runtime", async () => {
  setEnv({ ...goodProd, NEXT_RUNTIME: "edge", TRUSTED_PROXY_HOPS: undefined });
  await register();
});

const ip = (hops: string | undefined, xff: string | undefined) => {
  setEnv({ TRUSTED_PROXY_HOPS: hops });
  return rl.clientIp(new Headers(xff === undefined ? {} : { "x-forwarded-for": xff }));
};

test("clientIp: spoofed leftmost entries never change the selected address, for each hop count", () => {
  for (const spoof of ["6.6.6.6", "6.6.6.6, 7.7.7.7", "::1", "127.0.0.1, 10.0.0.1, 8.8.8.8"]) {
    // 1 hop: the proxy appended the real client as the last entry.
    assert.equal(ip("1", `${spoof}, 203.0.113.9`), "203.0.113.9");
    // 2 hops: client, then a CDN-seen address, then the inner proxy's peer.
    assert.equal(ip("2", `${spoof}, 203.0.113.9, 10.0.0.1`), "203.0.113.9");
    // 3 hops.
    assert.equal(ip("3", `${spoof}, 203.0.113.9, 10.0.0.1, 10.0.0.2`), "203.0.113.9");
  }
});

test("clientIp: different hop counts pick different entries and short chains fall to the unknown bucket", () => {
  const chain = "1.1.1.1, 2.2.2.2, 3.3.3.3";
  assert.equal(ip("1", chain), "3.3.3.3");
  assert.equal(ip("2", chain), "2.2.2.2");
  assert.equal(ip("3", chain), "1.1.1.1");
  assert.equal(ip("4", chain), rl.UNKNOWN_IP, "never falls back to the client-controlled first entry");
  assert.equal(ip("2", "1.1.1.1"), rl.UNKNOWN_IP);
  assert.equal(ip("1", undefined), rl.UNKNOWN_IP);
  assert.equal(ip("1", " , ,"), rl.UNKNOWN_IP);
  assert.equal(ip(undefined, "9.9.9.9, 4.4.4.4"), "4.4.4.4", "default is one hop");
});
