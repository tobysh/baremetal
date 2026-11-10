import test from "node:test";
import assert from "node:assert/strict";
import { safeNext } from "../../src/lib/redirects";

test("keeps same-site relative paths with query", () => {
  assert.equal(safeNext("/lessons/c/01-hello?tab=hints"), "/lessons/c/01-hello?tab=hints");
  assert.equal(safeNext("/"), "/");
});

test("rejects open-redirect shapes", () => {
  for (const bad of [
    "//evil.example", "///evil.example", "/\\evil.example", "\\\\evil.example", "https://evil.example", "http://evil.example/x",
    "javascript:alert(1)", "evil.example", "", "/ /evil", "/\t/evil.example", "/%5cevil.example", "/%0d%0aSet-Cookie:x=1", "/a\nb", "/\u0000",
  ]) assert.equal(safeNext(bad), "/dashboard", JSON.stringify(bad));
});

test("non-strings and custom fallback", () => {
  assert.equal(safeNext(undefined), "/dashboard");
  assert.equal(safeNext(["/x"]), "/dashboard");
  assert.equal(safeNext(null, "/home"), "/home");
  assert.equal(safeNext("//evil.example", "/home"), "/home");
});
