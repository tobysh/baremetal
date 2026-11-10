import test from "node:test";
import assert from "node:assert/strict";

// `harnessTampering` is module-private in src/lib/runner-engine.ts. These cases run once it is exported
// (`export function harnessTampering`); until then they are skipped and fixtures-forgery.mts is the coverage.
process.env.AUTH_SECRET ??= "unit-test-secret-unit-test-secret-0123456789";
const mod = (await import("../../src/lib/runner-engine")) as Record<string, unknown>;
const fn = mod.harnessTampering as ((code: string, lang: string) => string | null) | undefined;
const opts = { skip: fn ? false : "harnessTampering is not exported from runner-engine.ts" };

test("clean code passes", opts, () => {
  assert.equal(fn!("int main(void){return 0;}", "c"), null);
  assert.equal(fn!("fn main(){ println!(\"hi\"); }", "rust"), null);
});

test("rust include family and #[path] are flagged, in any spacing", opts, () => {
  for (const bad of ['const A: &str = include_str!("main.rs");', 'include_str !("main.rs")', "#[path = \"main.rs\"] mod m;", "include_bytes!(\"x\")"])
    assert.ok(fn!(bad, "rust"), bad);
});

test("asm directives are flagged case-insensitively", opts, () => {
  for (const bad of ['.incbin "tests.c"', ".INCBIN \"x\"", ".include \"x\"", ".macro m", ".IRP x", ".init_array"]) assert.ok(fn!(bad, "asm"), bad);
});

test("C23 #embed is flagged for c and kernel", opts, () => {
  assert.ok(fn!('const char d[] = {\n#embed "main.c"\n};', "c"));
  assert.ok(fn!("__has_embed(\"x\")", "kernel"));
});

test("protocol lines, quoted includes and /proc/self are flagged for every language", opts, () => {
  assert.ok(fn!('#include "tests.c"', "c"));
  assert.ok(fn!('open("/proc/self/exe")', "c"));
  assert.ok(fn!('//go:embed bm_test.go', "go"));
});
