import test from "node:test";
import assert from "node:assert/strict";
import { gradingTrust, isHighTrust } from "../../src/lib/grading-trust";

test("driver wins over mode", () => {
  assert.equal(gradingTrust({ mode: "harness", driver: "x" }), "driver");
});
test("stdout and harness", () => {
  assert.equal(gradingTrust({ mode: "stdout" }), "stdout");
  assert.equal(gradingTrust({ mode: "harness" }), "harness");
});
test("harness is not high trust", () => {
  assert.equal(isHighTrust("harness"), false);
  assert.equal(isHighTrust("driver"), true);
  assert.equal(isHighTrust("stdout"), true);
});
