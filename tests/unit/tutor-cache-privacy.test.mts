import test from "node:test";
import assert from "node:assert/strict";
import { hasUnvettedComments, privateWords, replyLeaks, wordsOf } from "../../src/lib/tutor-cache";

const pub = new Set(["int", "main", "return", "total"]);
const code = "int secret_balance = 0; // call mom at 555\nint main(void){ return secret_balance; }";

test("concept replies never saw the code, so nothing is private", () => {
  assert.equal(privateWords(code, "concept", "c", pub).size, 0);
});

test("failure replies may not repeat learner identifiers, in any case or punctuation", () => {
  const priv = privateWords(code, "failure", "c", pub);
  assert.ok(replyLeaks("Check `secret_balance` first.", priv));
  assert.ok(replyLeaks("SECRET_BALANCE is never updated", priv) || replyLeaks("secret_balance.", priv));
  assert.ok(!replyLeaks("The loop is off by one.", priv));
});

test("exact replies may not repeat comment text, and unvetted comments block sharing", () => {
  assert.ok(privateWords(code, "exact", "c", pub).has("mom"));
  assert.ok(hasUnvettedComments(code, "c", pub));
  assert.ok(!hasUnvettedComments("int main(void){ return 0; }", "c", pub));
});

test("short or empty private sets never flag", () => {
  assert.equal(replyLeaks("anything", new Set()), false);
  assert.equal(wordsOf("a b", 3).size, 0);
});
