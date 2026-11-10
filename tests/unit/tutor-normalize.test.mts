import test from "node:test";
import assert from "node:assert/strict";
import { normalizeCode, normalizeMessage, normalizeOutput } from "../../src/lib/tutor-normalize";
import { normalizeQuestion } from "../../src/lib/tutor-cache";

test("code: comments and whitespace do not change the key", () => {
  const a = "int main(void) {\n  // hi\n  return 0; /* x */\n}\n";
  const b = "int  main(void){\r\n\n\n return 0;\n}";
  assert.equal(normalizeCode(a, "c"), normalizeCode(b, "c"));
});

test("code: string literals are kept verbatim, including comment markers", () => {
  assert.ok(normalizeCode('puts("a  // b");', "c").includes('"a  // b"'));
  assert.notEqual(normalizeCode('puts("a");', "c"), normalizeCode('puts("b");', "c"));
});

test("code: a space between word characters is significant", () => {
  assert.notEqual(normalizeCode("int x;", "c"), normalizeCode("intx;", "c"));
});

test("code: asm treats # as a comment, C does not", () => {
  assert.equal(normalizeCode("mov $1, %rax # one\nret", "asm"), normalizeCode("mov $1, %rax\nret", "asm"));
  assert.ok(normalizeCode("#include <stdio.h>", "c").startsWith("#include"));
});

test("code: Go raw strings and Rust lifetimes", () => {
  assert.ok(normalizeCode("s := `a // b`", "go").includes("`a // b`"));
  assert.equal(normalizeCode("fn f<'a>(x: &'a str) {} // c", "rust"), normalizeCode("fn f<'a>(x: &'a str) {}", "rust"));
});

test("questions: C, C++ and C# keep separate cache keys", () => {
  assert.notEqual(normalizeQuestion("what is c++?"), normalizeQuestion("what is c?"));
  assert.notEqual(normalizeQuestion("what is c#?"), normalizeQuestion("what is c?"));
  assert.notEqual(normalizeQuestion("what is c#?"), normalizeQuestion("what is c++?"));
});

test("code: language-distinct questions are not conflated by the normaliser (known limitation of the +/# plan item)", () => {
  // Questions mentioning C++ / C# keep their punctuation only if normalizeMessage preserves it.
  assert.notEqual(normalizeMessage("what is c++?"), normalizeMessage("what is c?"));
  assert.notEqual(normalizeMessage("what is c#?"), normalizeMessage("what is c?"));
});

test("message: case, spacing and trailing punctuation", () => {
  assert.equal(normalizeMessage("  What   IS a pointer ?! "), "what is a pointer");
});

test("output: line endings, colour codes, trailing space and blank runs", () => {
  assert.equal(normalizeOutput("\u001b[31mfail\u001b[0m  \r\n\r\n\r\n\r\nok\t\n"), "fail\n\nok");
});
