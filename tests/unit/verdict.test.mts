import test from "node:test";
import assert from "node:assert/strict";
import { CHANNEL_TAMPERED, compareTranscripts, formatValue, parseTranscript, verdictReferenceKey } from "../../src/lib/verdict";

test("reference cache separates language, adapter and solution changes", () => {
  const ex = { language: "c" as const, driver: "driver", solution: "source" };
  const key = verdictReferenceKey(ex);
  assert.equal(verdictReferenceKey({ ...ex }), key);
  assert.notEqual(verdictReferenceKey({ ...ex, language: "asm" }), key);
  assert.notEqual(verdictReferenceKey({ ...ex, driver: "different" }), key);
  assert.notEqual(verdictReferenceKey({ ...ex, solution: "different" }), key);
});

const hex = (s: string) => Buffer.from(s).toString("hex");
const rec = (dir: "I" | "O", type: string, label: string, value: string) => `${dir} ${type} ${hex(label)} ${value}`;
const kase = (kind: "F" | "S", name: string, ...records: string[]) => [`C ${kind} ${hex(name)}`, ...records];
const transcript = (...lines: string[][]) => `${lines.flat().join("\n")}\nE\n`;

const fixed = (out: string) => kase("F", "swap 1 2", rec("I", "i", "x", "1"), rec("I", "i", "y", "2"), rec("O", "i", "x", out), rec("O", "i", "y", "1"));
const seeded = (input: string, out: string) => kase("S", "swap random", rec("I", "i", "x", input), rec("O", "i", "x", out));
const reference = parseTranscript(transcript(fixed("2"), seeded("41", "17")));

test("a well-formed transcript parses", () => {
  assert.equal(reference.valid, true);
  assert.equal(reference.complete, true);
  assert.deepEqual(reference.cases.map((c) => [c.name, c.fixed, c.records.length]), [["swap 1 2", true, 4], ["swap random", false, 2]]);
});

test("malformed or out-of-place lines make a transcript invalid", () => {
  for (const bad of [
    `${rec("O", "i", "x", "1")}\nE\n`, // a record before any check
    `${kase("F", "a").join("\n")}\nhello\nE\n`, // junk
    `${kase("F", "a").join("\n")}\nE\n${kase("F", "b").join("\n")}\n`, // something after the end
    `${kase("F", "a").join("\n")}\n${rec("O", "s", "s", "3:6162")}\nE\n`, // string length does not match its bytes
    `${kase("F", "a").join("\n")}\n${rec("O", "ai", "a", "3:1,2")}\nE\n`, // array length does not match
  ]) assert.equal(parseTranscript(bad).valid, false, bad);
});

test("identical transcripts pass every check", () => {
  const { results, intact } = compareTranscripts(reference, parseTranscript(transcript(fixed("2"), seeded("41", "17"))), null);
  assert.equal(intact, true);
  assert.ok(results.every((r) => r.passed));
});

test("a wrong fixed answer shows the expected value; a wrong seeded answer does not", () => {
  const { results, intact } = compareTranscripts(reference, parseTranscript(transcript(fixed("9"), seeded("41", "99"))), null);
  assert.equal(intact, true);
  assert.match(results[0].message ?? "", /Expected: x = 2, y = 1/);
  assert.match(results[1].message ?? "", /Input: x = 41/);
  assert.match(results[1].message ?? "", /Your result: x = 99/);
  assert.doesNotMatch(results[1].message ?? "", /Expected|17/);
});

test("changed inputs, missing, extra or renamed checks are not intact", () => {
  const changedInput = compareTranscripts(reference, parseTranscript(transcript(fixed("2"), seeded("5", "17"))), null);
  assert.equal(changedInput.intact, false);
  assert.equal(changedInput.results[1].message, CHANNEL_TAMPERED);
  const missing = compareTranscripts(reference, parseTranscript(transcript(fixed("2"))), null);
  assert.equal(missing.intact, false);
  assert.equal(missing.results[1].passed, false);
  const extra = compareTranscripts(reference, parseTranscript(transcript(fixed("2"), seeded("41", "17"), fixed("2"))), null);
  assert.equal(extra.intact, false);
  const renamed = compareTranscripts(reference, parseTranscript(transcript(fixed("2"), kase("S", "other", rec("I", "i", "x", "41"), rec("O", "i", "x", "17")))), null);
  assert.equal(renamed.intact, false);
});

test("a crash part-way through a check fails it with the crash and skips the rest", () => {
  const partial = parseTranscript(`${[...kase("F", "swap 1 2", rec("I", "i", "x", "1"))].join("\n")}\n`);
  const { results, intact } = compareTranscripts(reference, partial, "Segmentation fault");
  assert.equal(intact, false);
  assert.equal(results[0].message, "Segmentation fault");
  assert.match(results[1].message ?? "", /Not run/);
});

test("values are shown the way C code would write them", () => {
  assert.equal(formatValue({ type: "ai", value: "3:1,-2,3" }), "{1, -2, 3}");
  assert.equal(formatValue({ type: "s", value: `2:${hex("hi")}` }), '"hi"');
  assert.equal(formatValue({ type: "s", value: "-" }), "NULL");
  assert.equal(formatValue({ type: "f", value: "3ff8000000000000" }), "1.5");
  assert.equal(formatValue({ type: "f", value: "4000000000000000" }), "2.0");
});
