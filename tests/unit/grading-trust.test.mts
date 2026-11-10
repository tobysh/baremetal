import test from "node:test";
import assert from "node:assert/strict";
import { gradingTrust, isHighTrust } from "../../src/lib/grading-trust";

test("only C harness exercises use independent drivers", () => {
  assert.equal(gradingTrust({ language: "c", mode: "harness", driver: "x" }), "driver");
  for (const language of ["rust", "go", "asm", "kernel"] as const) {
    assert.equal(gradingTrust({ language, mode: "harness", driver: "x" }), "harness");
  }
  assert.equal(gradingTrust({ language: "c", mode: "stdout", driver: "x" }), "stdout");
});
test("stdout and harness", () => {
  assert.equal(gradingTrust({ language: "c", mode: "stdout" }), "stdout");
  assert.equal(gradingTrust({ language: "c", mode: "harness" }), "harness");
});
test("harness is not high trust", () => {
  assert.equal(isHighTrust("harness"), false);
  assert.equal(isHighTrust("driver"), true);
  assert.equal(isHighTrust("stdout"), true);
});
