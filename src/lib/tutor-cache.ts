// Pure helpers for the shared tutor caches. No server-only imports, so scripts can test them directly.
//
// Three kinds of cache entry live in `tutor_cache`:
//   exact    first question, same lesson/level/message/code/output (the original cache)
//   concept  a conceptual question that does not depend on the learner's code. The model
//            is called WITHOUT the code or output, so the answer cannot contain or depend on it.
//   failure  a generic "help / what went wrong" message with the same failing output and the
//            same code shape (identifiers renamed by first use, comments and whitespace ignored).
//
// Privacy rule for everything that is shared between learners: a reply that was written while
// looking at one learner's code is only stored if it contains none of that code's private words
// (identifiers, comment text) that are not already public lesson vocabulary.

import { normalizeCode, normalizeMessage, normalizeOutput } from "./tutor-normalize";

export type CacheKind = "exact" | "concept" | "failure";
export type QuestionClass = "concept" | "generic" | "specific";

/** Entries asked this many times are "popular": kept longer and listed for admins. */
export const POPULAR_HITS = 5;
export const CACHE_TTL_DAYS = 30;
export const POPULAR_TTL_DAYS = 90;

// ---------- Question classification (cheap heuristics, no model call) ----------

const CODE_CHARS = /[`{}();=<>[\]#\\]|\w\(|->|::|\/\/|\/\*|\d{3,}|https?:/;

/** Messages that only ask for help with whatever the code and output currently show. */
const GENERIC_HELP = new RegExp(
  "^(?:" +
    [
      "(?:i )?(?:do not|dont|don t) know (?:where|how) to (?:start|begin)",
      "(?:where|how) (?:do|should|can) i (?:start|begin)",
      "help(?: me| please)?",
      "(?:i am|im|i m) stuck",
      "stuck",
      "what is wrong(?: with (?:my|this|the) (?:code|solution|program|answer))?",
      "what s wrong(?: with (?:my|this|the) (?:code|solution|program|answer))?",
      "what am i doing wrong",
      "what went wrong",
      "why (?:does|is|did) (?:this|it|my code|my program|my solution) (?:fail|crash|break|not work|not working|failing)",
      "why (?:is|does) (?:this|it|my code) (?:wrong|failing|crashing)",
      "(?:my code|my program|this|it) (?:does not|doesn t|doesnt|is not|isn t|isnt) (?:work|working)",
      "(?:please )?(?:explain|what does) (?:this|the) (?:error|output|failure|message|crash)(?: mean)?",
      "what does (?:this|the) (?:error|failure|message) mean",
      "how (?:do|can|should) i fix (?:this|it|the error)",
      "fix (?:this|it)",
      "what now",
      "what next",
      "why am i getting this (?:error|output|result)",
      "why do i get this (?:error|output|result)",
    ].join("|") +
    ")$",
);

const FILLER_PREFIX = /^(?:(?:hi|hey|hello|ok|okay|so|please|pls|um|well)\b[ ,]*)+/;
const POLITE_PREFIX = /^(?:(?:can|could|would|will) you (?:please )?(?:tell me|explain|help me understand|show me)?|please|pls)\b[ ,]*/;

/** Lowercases, drops punctuation and polite filler, so "Hey, can you explain what's a pointer??" matches "what is a pointer". */
export function normalizeQuestion(message: string): string {
  return normalizeMessage(message)
    .replace(/['’]s\b/g, " is")
    .replace(/['’]/g, "")
    .replace(/[^a-z0-9 ]+/g, " ")
    .replace(/\s+/g, " ")
    .trim()
    .replace(FILLER_PREFIX, "")
    .replace(POLITE_PREFIX, "")
    .replace(/\s+/g, " ")
    .trim();
}

const QUESTION_START = /^(?:what|why|how|when|which|where|can|does|do|is|are|explain|describe|define|difference|whats|tell me about|compare)\b/;
// Anything that refers to the learner, their code, or the exercise at hand is not a general question.
const SELF_REFERENCE =
  /\b(?:my|mine|me|myself|we|our|i|im|ive|id|ill|this|these|those|that|it|its|here|there|above|below|line|lines|error|errors|output|outputs|code|program|solution|exercise|task|test|tests|expected|got|result|answer|function here|variable here|wrong|bug|stuck)\b/;

export function classifyQuestion(message: string): QuestionClass {
  if (message.length > 200 || CODE_CHARS.test(message)) return "specific";
  const q = normalizeQuestion(message);
  if (GENERIC_HELP.test(q)) return "generic";
  // "how do I ..." is a general question as long as nothing else points at the learner's own code.
  const general = q.replace(/^(how (?:do|can|would|should)) i\b/, "$1 you");
  const words = general.split(" ").length;
  if (words >= 2 && words <= 30 && QUESTION_START.test(general) && !SELF_REFERENCE.test(general)) return "concept";
  return "specific";
}

// ---------- Output and code fingerprints ----------

/** Failing output with the parts that vary between runs (addresses, temp paths, timings, pids) masked. */
export function normalizeFailureOutput(output: string): string {
  return normalizeOutput(output)
    .replace(/0x[0-9a-fA-F]{4,}/g, "0xADDR")
    .replace(/\/(?:tmp|work|sandbox|run)\/[^\s:'"]+/g, "/PATH")
    .replace(/\b\d+(?:\.\d+)?\s?(?:ms|µs|us|ns|s)\b/g, "TIME")
    .replace(/\bpid[ =:]\d+/gi, "pid N");
}

export function wordsOf(text: string, minLength = 1): Set<string> {
  const out = new Set<string>();
  for (const m of text.matchAll(/[A-Za-z_][A-Za-z0-9_]*/g)) if (m[0].length >= minLength) out.add(m[0].toLowerCase());
  return out;
}

const KEYWORDS = new Set(
  (
    // C / kernel C
    "auto break case char const continue default do double else enum extern float for goto if inline int long register restrict return short signed sizeof static struct switch typedef union unsigned void volatile while bool true false null nullptr size_t ssize_t file uintptr_t intptr_t ptrdiff_t main asm " +
    // Rust
    "as async await crate dyn fn impl in let loop match mod move mut pub ref self super trait type unsafe use where i8 i16 i32 i64 i128 isize u8 u16 u32 u64 u128 usize f32 f64 str string vec option some none result ok err box rc arc " +
    // Go
    "chan defer fallthrough func go import interface map package range select var byte rune error int8 int16 int32 int64 uint uint8 uint16 uint32 uint64 uintptr float32 float64 complex64 complex128 any nil iota make new len cap append copy delete panic recover print println"
  ).split(" "),
);

/**
 * A fingerprint of what the code does, not what it is called. Comments and layout are ignored;
 * identifiers that are not public lesson vocabulary and are not called as functions are renamed in order
 * of first use (so `a = b; b = a` and `x = y; y = x` match). String literals and numbers are kept, because
 * they decide the behaviour. Assembly is not abstracted: registers and mnemonics are the program.
 */
export function codeShape(code: string, language: string, publicWords: Set<string>): string {
  const src = normalizeCode(code, language);
  if (language === "asm") return src;
  const quote = language === "rust" ? `"` : language === "go" ? `"'\`` : `"'`;
  const tokens: string[] = [];
  const names = new Map<string, string>();
  let i = 0;
  while (i < src.length) {
    const c = src[i];
    if (quote.includes(c)) {
      let j = i + 1;
      while (j < src.length && src[j] !== c && (src[j] !== "\n" || c === "`")) j += src[j] === "\\" && c !== "`" ? 2 : 1;
      tokens.push(src.slice(i, j + 1));
      i = j + 1;
    } else if (/[A-Za-z_]/.test(c)) {
      let j = i + 1;
      while (j < src.length && /\w/.test(src[j])) j++;
      const id = src.slice(i, j);
      const lower = id.toLowerCase();
      let k = j;
      while (src[k] === " ") k++;
      const keep = KEYWORDS.has(lower) || publicWords.has(lower) || /_t$/.test(id) || src[k] === "(" || src[k] === "!";
      if (keep) tokens.push(id);
      else {
        if (!names.has(id)) names.set(id, `$${names.size}`);
        tokens.push(names.get(id)!);
      }
      i = j;
    } else if (/\d/.test(c)) {
      let j = i + 1;
      while (j < src.length && /[\w.]/.test(src[j])) j++;
      tokens.push(src.slice(i, j));
      i = j;
    } else {
      tokens.push(c === "\n" ? "\\n" : c);
      i++;
    }
  }
  return tokens.join(" ");
}

// ---------- Privacy checks ----------

/** Words (3+ letters) that appear in comments in the code but not in the comment-free code. */
export function commentWords(code: string, language: string): Set<string> {
  const inCode = wordsOf(normalizeCode(code, language), 3);
  const out = new Set<string>();
  for (const w of wordsOf(code, 3)) if (!inCode.has(w)) out.add(w);
  return out;
}

/** Words from the code that are neither public lesson vocabulary nor common to every learner. */
export function privateWords(code: string, kind: CacheKind, language: string, publicWords: Set<string>): Set<string> {
  if (kind === "concept") return new Set(); // the model never saw the code
  const source = kind === "exact" ? commentWords(code, language) : wordsOf(code, 3);
  const out = new Set<string>();
  for (const w of source) if (!publicWords.has(w)) out.add(w);
  return out;
}

/**
 * Comment text the learner wrote is invisible to the cache keys but visible to the model.
 * A reply written next to unvetted comments (which could contain instructions meant for the model)
 * is never shared.
 */
export function hasUnvettedComments(code: string, language: string, publicWords: Set<string>): boolean {
  for (const w of commentWords(code, language)) if (!publicWords.has(w)) return true;
  return false;
}

/** True when the reply repeats any private word of the author's code. */
export function replyLeaks(reply: string, priv: Set<string>): boolean {
  if (!priv.size) return false;
  for (const w of wordsOf(reply, 3)) if (priv.has(w)) return true;
  return false;
}

/** Cutoff for entries that are asked often. */
export const expiryFor = (hits: number, now = Date.now()) => new Date(now + (hits >= POPULAR_HITS ? POPULAR_TTL_DAYS : CACHE_TTL_DAYS) * 86_400_000);
