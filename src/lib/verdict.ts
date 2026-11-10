// Out-of-process verdict for C lessons that ship a `driver.c` (docs/security.md L1).
//
// The driver is compiled as its own translation unit and linked with the learner's object. It
// calls the learner's functions with fixed and seeded inputs and writes typed records to fd 3.
// The same driver is linked with the reference solution and run with the same seed in its own
// sandbox. The runner then compares the two transcripts record by record, so the pass/fail
// decision is never taken inside the learner's process: forging a passing transcript means
// producing the reference's outputs for inputs drawn from a fresh 16-byte seed, which is
// solving the exercise.
//
// Wire format, one record per line (all names, labels and strings are lowercase hex):
//   C <F|S> <name>            a check starts; F = fixed edge case, S = seeded case
//   I <type> <label> <value>  an input the driver passed
//   O <type> <label> <value>  a value observed after the call
//   E                         the driver finished
// Types: i (signed decimal), u (unsigned decimal), f (IEEE-754 double as 16 hex digits),
// s (`<len>:<hex>` or `-` for NULL), b (`<len>:<hex>`), ai / au (`<n>:v1,v2,...`).
import type { TestCaseResult } from "./runner-types";

/**
 * Prepended to every driver. Self-contained on purpose: the record writer uses raw syscalls and
 * its own number formatting, so a learner who defines `write` or `snprintf` can't intercept it.
 * The seed arrives on fd 4 and the result pipe on fd 5; a .preinit_array hook (which runs before
 * any constructor, so before learner code) reads and closes the seed, moves the pipe to fd 3 and
 * wipes its copy of the seed once the generator state is derived.
 */
export const VERDICT_PRELUDE = String.raw`
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#define BM_UNUSED_ __attribute__((unused))
static long bm_sys_(long n, long a, long b, long c) {
  long r;
  __asm__ volatile("syscall" : "=a"(r) : "a"(n), "D"(a), "S"(b), "d"(c) : "rcx", "r11", "memory");
  return r;
}
static __attribute__((noreturn)) void bm_die_(const char *m) {
  size_t n = 0;
  while (m[n]) n++;
  bm_sys_(1, 2, (long)m, (long)n);
  for (;;) bm_sys_(231, 70, 0, 0);
}
static uint64_t bm_state_;
static void bm_take_seed_(int argc, char **argv, char **envp) {
  (void)argc; (void)argv; (void)envp;
  volatile unsigned char seed[16];
  size_t got = 0;
  while (got < sizeof seed) {
    long r = bm_sys_(0, 4, (long)(seed + got), (long)(sizeof seed - got));
    if (r <= 0) bm_die_("bm: the test seed is missing\n");
    got += (size_t)r;
  }
  bm_sys_(3, 4, 0, 0);
  if (bm_sys_(33, 5, 3, 0) != 3) bm_die_("bm: the result channel is missing\n");
  bm_sys_(3, 5, 0, 0);
  uint64_t a = 0, b = 0;
  for (int i = 0; i < 8; i++) { a = a << 8 | seed[i]; b = b << 8 | seed[8 + i]; }
  bm_state_ = a ^ (b * 0xd1342543de82ef95ull);
  for (size_t i = 0; i < sizeof seed; i++) seed[i] = 0;
}
__attribute__((section(".preinit_array"), used)) static void (*bm_hook_)(int, char **, char **) = bm_take_seed_;
__attribute__((constructor)) static void bm_stdio_(void) { setvbuf(stdout, NULL, _IOLBF, 0); }

/* splitmix64 */
static BM_UNUSED_ uint64_t bm_rand(void) {
  uint64_t z = (bm_state_ += 0x9e3779b97f4a7c15ull);
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
  z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
  return z ^ (z >> 31);
}
/* A uniform-enough integer in [lo, hi], both inclusive. */
static BM_UNUSED_ int64_t bm_int(int64_t lo, int64_t hi) {
  uint64_t span = (uint64_t)hi - (uint64_t)lo + 1;
  return span ? (int64_t)((uint64_t)lo + bm_rand() % span) : (int64_t)bm_rand();
}
#define bm_fill(arr, n, lo, hi) do { for (size_t bm_f_ = 0; bm_f_ < (size_t)(n); bm_f_++) (arr)[bm_f_] = bm_int((lo), (hi)); } while (0)

static char bm_buf_[1 << 16];
static size_t bm_len_;
static void bm_ch_(char c) { if (bm_len_ >= sizeof bm_buf_) bm_die_("bm: record too long\n"); bm_buf_[bm_len_++] = c; }
static size_t bm_slen_(const char *s) { size_t n = 0; while (s[n]) n++; return n; }
static void bm_hex_(const void *p, size_t n) {
  const unsigned char *b = (const unsigned char *)p;
  for (size_t i = 0; i < n; i++) { bm_ch_("0123456789abcdef"[b[i] >> 4]); bm_ch_("0123456789abcdef"[b[i] & 15]); }
}
static void bm_u_(uint64_t v) { char t[24]; int k = 0; do { t[k++] = (char)('0' + v % 10); v /= 10; } while (v); while (k) bm_ch_(t[--k]); }
static void bm_i_(int64_t v) { if (v < 0) { bm_ch_('-'); bm_u_((uint64_t)0 - (uint64_t)v); } else bm_u_((uint64_t)v); }
static void bm_flush_(void) {
  bm_ch_('\n');
  size_t off = 0;
  while (off < bm_len_) {
    long r = bm_sys_(1, 3, (long)(bm_buf_ + off), (long)(bm_len_ - off));
    if (r <= 0) bm_die_("bm: the result channel was closed\n");
    off += (size_t)r;
  }
  bm_len_ = 0;
}
static void bm_head_(char dir, const char *type, const char *label) {
  bm_len_ = 0; bm_ch_(dir); bm_ch_(' ');
  while (*type) bm_ch_(*type++);
  bm_ch_(' '); bm_hex_(label, bm_slen_(label)); bm_ch_(' ');
}
static BM_UNUSED_ void bm_case_(const char *name, char kind) { bm_len_ = 0; bm_ch_('C'); bm_ch_(' '); bm_ch_(kind); bm_ch_(' '); bm_hex_(name, bm_slen_(name)); bm_flush_(); }
static BM_UNUSED_ void bm_rec_i_(char d, const char *l, int64_t v) { bm_head_(d, "i", l); bm_i_(v); bm_flush_(); }
static BM_UNUSED_ void bm_rec_u_(char d, const char *l, uint64_t v) { bm_head_(d, "u", l); bm_u_(v); bm_flush_(); }
static BM_UNUSED_ void bm_rec_f_(char d, const char *l, double v) {
  union { double d; uint64_t u; } x; x.d = v;
  bm_head_(d, "f", l);
  for (int s = 60; s >= 0; s -= 4) bm_ch_("0123456789abcdef"[(x.u >> s) & 15]);
  bm_flush_();
}
static BM_UNUSED_ void bm_rec_b_(char d, const char *type, const char *l, const void *p, size_t n) {
  bm_head_(d, type, l);
  if (!p) bm_ch_('-'); else { bm_u_(n); bm_ch_(':'); bm_hex_(p, n); }
  bm_flush_();
}
#define BM_ARR_(d, type, conv, label, arr, n) do { \
  size_t bm_n_ = (size_t)(n); bm_head_((d), (type), (label)); bm_u_(bm_n_); bm_ch_(':'); \
  for (size_t bm_k_ = 0; bm_k_ < bm_n_; bm_k_++) { if (bm_k_) bm_ch_(','); conv((arr)[bm_k_]); } \
  bm_flush_(); } while (0)
#define BM_CONV_I_(v) bm_i_((int64_t)(v))
#define BM_CONV_U_(v) bm_u_((uint64_t)(v))

#define BM_CASE(name) bm_case_((name), 'F')
#define BM_SEEDED(name) bm_case_((name), 'S')
#define BM_IN_I(label, v) bm_rec_i_('I', (label), (int64_t)(v))
#define BM_OUT_I(label, v) bm_rec_i_('O', (label), (int64_t)(v))
#define BM_IN_U(label, v) bm_rec_u_('I', (label), (uint64_t)(v))
#define BM_OUT_U(label, v) bm_rec_u_('O', (label), (uint64_t)(v))
#define BM_IN_F(label, v) bm_rec_f_('I', (label), (double)(v))
#define BM_OUT_F(label, v) bm_rec_f_('O', (label), (double)(v))
#define BM_IN_S(label, s) do { const char *bm_s_ = (s); bm_rec_b_('I', "s", (label), bm_s_, bm_s_ ? bm_slen_(bm_s_) : 0); } while (0)
#define BM_OUT_S(label, s) do { const char *bm_s_ = (s); bm_rec_b_('O', "s", (label), bm_s_, bm_s_ ? bm_slen_(bm_s_) : 0); } while (0)
#define BM_IN_BYTES(label, p, n) bm_rec_b_('I', "b", (label), (p), (size_t)(n))
#define BM_OUT_BYTES(label, p, n) bm_rec_b_('O', "b", (label), (p), (size_t)(n))
#define BM_IN_ARR(label, arr, n) BM_ARR_('I', "ai", BM_CONV_I_, (label), (arr), (n))
#define BM_OUT_ARR(label, arr, n) BM_ARR_('O', "ai", BM_CONV_I_, (label), (arr), (n))
#define BM_IN_UARR(label, arr, n) BM_ARR_('I', "au", BM_CONV_U_, (label), (arr), (n))
#define BM_OUT_UARR(label, arr, n) BM_ARR_('O', "au", BM_CONV_U_, (label), (arr), (n))

#define BM_DRIVER static void bm_driver_(void)
static void bm_driver_(void);
int main(void) { bm_driver_(); bm_len_ = 0; bm_ch_('E'); bm_flush_(); return 0; }
`;

export type VerdictRecord = { dir: "I" | "O"; type: string; label: string; value: string };
export type VerdictCase = { name: string; fixed: boolean; records: VerdictRecord[] };
/** `complete`: the driver reached its end record. `valid`: every line parsed and nothing followed the end record. */
export type Transcript = { cases: VerdictCase[]; complete: boolean; valid: boolean };

const HEX = /^(?:[0-9a-f]{2})*$/;
const fromHex = (h: string) => Buffer.from(h, "hex").toString("utf8");
const LENGTH_HEX = /^(\d{1,7}):((?:[0-9a-f]{2})*)$/;
const VALUE: Record<string, (v: string) => boolean> = {
  i: (v) => /^-?\d{1,20}$/.test(v),
  u: (v) => /^\d{1,20}$/.test(v),
  f: (v) => /^[0-9a-f]{16}$/.test(v),
  s: (v) => v === "-" || lengthMatches(v),
  b: (v) => v === "-" || lengthMatches(v),
  ai: (v) => arrayMatches(v, /^-?\d{1,20}$/),
  au: (v) => arrayMatches(v, /^\d{1,20}$/),
};
function lengthMatches(v: string) {
  const m = LENGTH_HEX.exec(v);
  return !!m && Number(m[1]) * 2 === m[2].length;
}
function arrayMatches(v: string, item: RegExp) {
  const m = /^(\d{1,7}):(.*)$/.exec(v);
  if (!m) return false;
  const items = m[2] === "" ? [] : m[2].split(",");
  return items.length === Number(m[1]) && items.every((x) => item.test(x));
}

/** Parses a transcript. Parsing stops at the first malformed line, which makes it invalid. */
export function parseTranscript(text: string): Transcript {
  const cases: VerdictCase[] = [];
  let complete = false;
  const lines = text.split("\n");
  if (lines[lines.length - 1] === "") lines.pop();
  for (const line of lines) {
    if (complete) return { cases, complete, valid: false };
    if (line === "E") { complete = true; continue; }
    const c = /^C ([FS]) ([0-9a-f]{2,2048})$/.exec(line);
    if (c && HEX.test(c[2])) { cases.push({ name: fromHex(c[2]), fixed: c[1] === "F", records: [] }); continue; }
    const r = /^([IO]) (i|u|f|s|b|ai|au) ([0-9a-f]{2,512}) (\S{1,200000})$/.exec(line);
    const current = cases[cases.length - 1];
    if (!r || !current || !HEX.test(r[3]) || !VALUE[r[2]](r[4])) return { cases, complete, valid: false };
    current.records.push({ dir: r[1] as "I" | "O", type: r[2], label: fromHex(r[3]), value: r[4] });
  }
  return { cases, complete, valid: true };
}

const MAX_SHOWN = 24;
/** A record's value as a learner would write it: numbers, C string literals, arrays in braces. */
export function formatValue(rec: Pick<VerdictRecord, "type" | "value">): string {
  const { type, value } = rec;
  if (type === "i" || type === "u") return value;
  if (type === "f") {
    const d = Buffer.from(value, "hex").readDoubleBE(0);
    return Number.isNaN(d) ? "NaN" : Object.is(d, -0) ? "-0.0" : Number.isInteger(d) && Math.abs(d) < 1e21 ? d.toFixed(1) : String(d);
  }
  if (type === "s" || type === "b") {
    if (value === "-") return "NULL";
    const [, hex] = value.split(":");
    const bytes = Buffer.from(hex, "hex");
    if (type === "b") return `{${[...bytes.subarray(0, MAX_SHOWN)].map((x) => `0x${x.toString(16).padStart(2, "0")}`).join(", ")}${bytes.length > MAX_SHOWN ? ", ..." : ""}} (${bytes.length} bytes)`;
    return JSON.stringify(bytes.toString("latin1"));
  }
  const [, list] = value.split(":");
  const items = list === "" ? [] : list.split(",");
  return `{${items.slice(0, MAX_SHOWN).join(", ")}${items.length > MAX_SHOWN ? `, ... (${items.length} values)` : ""}}`;
}

const describe = (records: VerdictRecord[]) => records.map((r) => `${r.label} = ${formatValue(r)}`).join(", ") || "(nothing)";
const sameRecord = (a: VerdictRecord, b: VerdictRecord) => a.dir === b.dir && a.type === b.type && a.label === b.label && a.value === b.value;
const sameShape = (a: VerdictRecord, b: VerdictRecord) => a.dir === b.dir && a.type === b.type && a.label === b.label;

export const CHANNEL_TAMPERED = "Something other than the tests wrote to the tests' result channel or changed the test inputs while your code ran, so this check can't be judged.";

/**
 * Compares the learner's transcript with the reference one, check by check. A missing, extra,
 * reordered or renamed check fails. `crash` explains a learner run that stopped early: the
 * check it stopped in gets that message and later checks are reported as not run. Seeded checks
 * show the input and the learner's values only; fixed edge cases also show the expected values.
 */
export function compareTranscripts(reference: Transcript, learner: Transcript, crash: string | null): { results: TestCaseResult[]; intact: boolean } {
  const results: TestCaseResult[] = [];
  let intact = learner.valid && learner.complete && learner.cases.length === reference.cases.length;
  let stopped = false;
  reference.cases.forEach((want, i) => {
    const got = learner.cases[i];
    if (!got) {
      results.push({ name: want.name, passed: false, message: stopped ? "Not run: the program stopped during an earlier check." : (crash ?? (learner.valid ? "The program stopped before this check ran." : CHANNEL_TAMPERED)) });
      stopped = true;
      return;
    }
    if (got.name !== want.name || got.fixed !== want.fixed) {
      intact = false;
      results.push({ name: want.name, passed: false, message: CHANNEL_TAMPERED });
      return;
    }
    const lastSeen = i === learner.cases.length - 1;
    const inputsWant = want.records.filter((r) => r.dir === "I"), inputsGot = got.records.filter((r) => r.dir === "I");
    const shapeOk = got.records.length <= want.records.length && got.records.every((r, k) => sameShape(r, want.records[k]));
    const inputsOk = inputsGot.length === inputsWant.length && inputsGot.every((r, k) => sameRecord(r, inputsWant[k]));
    if (got.records.length === want.records.length && got.records.every((r, k) => sameRecord(r, want.records[k]))) {
      results.push({ name: want.name, passed: true });
      return;
    }
    if (lastSeen && !learner.complete && got.records.length < want.records.length && shapeOk) {
      // The program stopped part-way through this check (a crash, a timeout or an exit).
      results.push({ name: want.name, passed: false, message: crash ?? (learner.valid ? "The program stopped during this check." : CHANNEL_TAMPERED) });
      stopped = true;
      return;
    }
    if (!shapeOk || !inputsOk || got.records.length !== want.records.length) {
      intact = false;
      results.push({ name: want.name, passed: false, message: CHANNEL_TAMPERED });
      return;
    }
    const outWant = want.records.filter((r) => r.dir === "O"), outGot = got.records.filter((r) => r.dir === "O");
    const actual = describe(outGot);
    const expected = describe(outWant);
    const input = describe(inputsWant);
    results.push(want.fixed
      ? { name: want.name, passed: false, message: `Input: ${input}\nExpected: ${expected}\nYour result: ${actual}` }
      : { name: want.name, passed: false, message: `Input: ${input}\nYour result: ${actual}\nThis check uses fresh random inputs on every run; work through this input by hand to find the mistake.` });
  });
  if (learner.cases.length > reference.cases.length) intact = false;
  return { results, intact };
}
