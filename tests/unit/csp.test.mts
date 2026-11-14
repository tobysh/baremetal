import test from "node:test";
import assert from "node:assert/strict";
import { mkdtemp, mkdir, rm, symlink, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import path from "node:path";
import { contentSecurityPolicy, scriptHash } from "../../src/lib/csp";
import { loadShellScriptHashes } from "../../src/lib/csp-shells";

async function fixture(run: (root: string) => Promise<void>) {
  const root = await mkdtemp(path.join(tmpdir(), "bm-csp-"));
  try { await run(root); } finally { await rm(root, { recursive: true, force: true }); }
}

test("production policies never allow arbitrary inline scripts", () => {
  for (const options of [{}, { nonce: "test-nonce" }, { scriptHashes: [scriptHash("theme()")] }]) {
    const script = contentSecurityPolicy(options).split(";").find((d) => d.trim().startsWith("script-src"));
    assert.ok(script);
    assert.ok(!script.includes("'unsafe-inline'"));
    assert.ok(!script.includes("'unsafe-eval'"));
  }
});

test("shell allowlist includes nested executable scripts and deduplicates them", async () => {
  await fixture(async (root) => {
    await mkdir(path.join(root, "nested"));
    await writeFile(path.join(root, "index.html"), '<script>theme()</script><script src="/chunk.js"></script><script type="application/json">{"data":1}</script>');
    await writeFile(path.join(root, "nested", "page.html"), '<script>theme()</script><script>reveal()</script>');
    assert.deepEqual(await loadShellScriptHashes(root), [scriptHash("theme()"), scriptHash("reveal()")].sort());
  });
});

test("missing and empty builds fail closed", async () => {
  await fixture(async (root) => {
    await assert.rejects(loadShellScriptHashes(path.join(root, "missing")));
    await assert.rejects(loadShellScriptHashes(root), /No executable scripts/);
    await writeFile(path.join(root, "index.html"), "<html></html>");
    await assert.rejects(loadShellScriptHashes(root), /No executable scripts/);
  });
});

test("a shell read failure rejects the entire allowlist", async () => {
  await fixture(async (root) => {
    await writeFile(path.join(root, "a.html"), "<script>theme()</script>");
    await symlink(path.join(root, "missing"), path.join(root, "z.html"));
    await assert.rejects(loadShellScriptHashes(root));
  });
});

test("proxy returns an uncached strict 503 when the build cannot be read", async () => {
  const { NextRequest } = await import("next/server");
  const { proxy } = await import("../../src/proxy");
  const originalDirectory = process.cwd();
  const errors: unknown[][] = [];
  const originalError = console.error;
  await fixture(async (root) => {
    try {
      process.chdir(root);
      console.error = (...args: unknown[]) => { errors.push(args); };
      for (let attempt = 0; attempt < 2; attempt++) {
        const response = await proxy(new NextRequest("https://baremetal.example/learn"));
        assert.equal(response.status, 503);
        assert.equal(response.headers.get("cache-control"), "no-store");
        assert.equal(response.headers.get("content-security-policy"), contentSecurityPolicy());
        assert.equal(await response.text(), "Temporarily unavailable");
      }
      assert.equal(errors.length, 1, "log once per process instead of on every failed request");
      assert.match(String(errors[0][0]), /Restore the complete .next build and restart/);
    } finally {
      process.chdir(originalDirectory);
      console.error = originalError;
    }
  });
});
