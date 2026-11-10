// Practice questions (parsons, blanks, predict, bug, choice, typed, match). No fs or server imports: the types and the
// browser-safe PublicPractice can be imported anywhere, while Practice (with answers) stays on the server.
import { practiceIdOf, practiceXpFor, xpFor, type Difficulty } from "../gamification";

export const PRACTICE_KINDS = ["parsons", "blanks", "predict", "bug", "choice", "typed", "match"] as const;
export type PracticeKind = (typeof PRACTICE_KINDS)[number];

type Common = {
  /** Short markdown instruction. */
  prompt: string;
  /** Markdown shown after a correct answer. */
  explain?: string;
};

/** Parsons and blanks assemble source code. By default that is the whole file and is run against the lesson's hidden tests when the order is not the canonical one. */
type Assembling = {
  /** Source around the assembled part, with `{{code}}` marking where it goes (parsons only; blanks use the template as the whole file). */
  base?: string;
  /** False when the assembled fragment cannot be run through the lesson's tests, so only the canonical answer counts. Default true. */
  runTests?: boolean;
};

export type ParsonsPractice = Common & Assembling & { kind: "parsons"; lines: string[]; distractors: string[] };
export type BlanksPractice = Common & Assembling & { kind: "blanks"; template: string; tokens: string[]; answers: string[] };
export type PredictPractice = Common & { kind: "predict"; code?: string; choices?: string[]; answer?: number; trace?: { label: string; choices: string[]; answer: number }[] };
export type BugPractice = Common & { kind: "bug"; code: string; bugLine: number };
/** Concept question. One correct index is single choice; several is "select all that apply". */
export type ChoicePractice = Common & { kind: "choice"; code?: string; choices: string[]; answers: number[] };
/** Free recall: the learner types a short answer (a value, a keyword, an instruction). Any entry of `accepted` matches after trimming and collapsing spaces. */
export type TypedPractice = Common & { kind: "typed"; code?: string; accepted: string[]; caseSensitive?: boolean };
/** Match each term on the left with its partner on the right. */
export type MatchPractice = Common & { kind: "match"; pairs: { left: string; right: string }[] };
/** Server-side practice, including its answers. Never send this to the browser. */
export type Practice = ParsonsPractice | BlanksPractice | PredictPractice | BugPractice | ChoicePractice | TypedPractice | MatchPractice;

/** What the browser may see: no answers, no canonical order, no blank values. `id` is the practice id (`<lesson id>#practice`). */
export type PublicPractice = { id: string; kind: PracticeKind; prompt: string; xp: number; fullXp: number } & (
  | { kind: "parsons"; lines: string[]; hasDistractors: boolean }
  | { kind: "blanks"; template: string; tokens: string[]; slots: number }
  | { kind: "predict"; code?: string; choices?: string[]; trace?: { label: string; choices: string[] }[] }
  | { kind: "bug"; code: string }
  | { kind: "choice"; code?: string; choices: string[]; multi: boolean }
  | { kind: "typed"; code?: string }
  | { kind: "match"; lefts: string[]; rights: string[] }
);

/**
 * What the browser submits as `submission` to POST /api/practice:
 * parsons: string[] (chosen lines in order); blanks: string[] (token per slot, in slot order);
 * predict: number (choice index) or, for a trace, number[] (one index per step); bug: number (1-based line);
 * choice: number[] (chosen indices); typed: string; match: string[] (the right-hand text chosen for each left, in order).
 */
export type PracticeSubmission = string[] | string | number | number[];

const SLOT = /\{\{(\d+)\}\}/g;
const MAX_LINES = 60, MAX_TEXT = 8_000;
export const slotCount = (template: string) => new Set([...template.matchAll(SLOT)].map((m) => Number(m[1]))).size;

const isStr = (v: unknown): v is string => typeof v === "string";
const strList = (v: unknown): v is string[] => Array.isArray(v) && v.every(isStr);
const isIndex = (v: unknown, n: number): v is number => Number.isInteger(v) && (v as number) >= 0 && (v as number) < n;

/** Validates a parsed practice.json. Throws a message naming the problem; `where` is the lesson folder. */
export function parsePractice(raw: unknown, where: string): Practice {
  const bad = (msg: string): never => { throw new Error(`${where}/practice.json: ${msg}`); };
  if (!raw || typeof raw !== "object" || Array.isArray(raw)) return bad("must be a JSON object");
  const r = raw as Record<string, unknown>;
  if (!PRACTICE_KINDS.includes(r.kind as PracticeKind)) return bad(`kind must be one of ${PRACTICE_KINDS.join(", ")}`);
  if (!isStr(r.prompt) || !r.prompt.trim() || r.prompt.length > 600) return bad("prompt must be a non-empty string of at most 600 characters");
  if (r.explain !== undefined && (!isStr(r.explain) || r.explain.length > 1200)) return bad("explain must be a string of at most 1200 characters");
  if (r.runTests !== undefined && typeof r.runTests !== "boolean") return bad("runTests must be a boolean");
  if (r.base !== undefined && (!isStr(r.base) || (r.base.match(/\{\{code\}\}/g) ?? []).length !== 1)) return bad("base must be a string containing {{code}} exactly once");
  const common = { prompt: r.prompt.trim(), explain: r.explain?.toString().trim() || undefined };
  const asm = { base: r.base as string | undefined, runTests: r.runTests as boolean | undefined };
  const lengthOk = (xs: string[]) => xs.length <= MAX_LINES && xs.every((x) => x.length <= 300);

  switch (r.kind) {
    case "parsons": {
      if (!strList(r.lines) || r.lines.length < 2 || !lengthOk(r.lines) || r.lines.some((l) => !l.trim())) return bad(`lines must be 2-${MAX_LINES} non-blank strings`);
      const distractors = r.distractors === undefined ? [] : r.distractors;
      if (!strList(distractors) || !lengthOk(distractors)) return bad("distractors must be an array of strings");
      const correct = new Set(r.lines.map((l) => l.trim()));
      for (const d of distractors) if (!d.trim() || correct.has(d.trim())) return bad(`distractor ${JSON.stringify(d)} is blank or also a correct line`);
      return { kind: "parsons", ...common, ...asm, lines: r.lines, distractors };
    }
    case "blanks": {
      if (!isStr(r.template) || r.template.length > MAX_TEXT) return bad("template must be a string");
      const slots = [...r.template.matchAll(SLOT)].map((m) => Number(m[1]));
      const n = slots.length;
      if (n < 1 || n > 12 || slots.slice().sort((a, b) => a - b).some((v, i) => v !== i) || new Set(slots).size !== n) return bad("template needs each of {{0}}..{{n-1}} exactly once (at most 12)");
      if (!strList(r.answers) || r.answers.length !== n) return bad(`answers must be ${n} strings, one per slot`);
      if (!strList(r.tokens) || r.tokens.length > 30 || !lengthOk(r.tokens)) return bad("tokens must be an array of strings");
      if (r.answers.some((a) => !a)) return bad("answers must not be empty");
      const pool = [...r.tokens];
      for (const a of r.answers) { const i = pool.indexOf(a); if (i < 0) return bad(`answer ${JSON.stringify(a)} is missing from tokens (the tray must hold one token per slot)`); pool.splice(i, 1); }
      return { kind: "blanks", ...common, ...asm, template: r.template, tokens: r.tokens, answers: r.answers };
    }
    case "predict": {
      if (r.code !== undefined && (!isStr(r.code) || r.code.length > MAX_TEXT)) return bad("code must be a string");
      const hasChoice = r.choices !== undefined, hasTrace = r.trace !== undefined;
      if (hasChoice === hasTrace) return bad("give either choices + answer or trace");
      if (hasChoice) {
        if (!strList(r.choices) || r.choices.length < 2 || r.choices.length > 8) return bad("choices must be 2-8 strings");
        if (new Set(r.choices).size !== r.choices.length) return bad("choices must be distinct");
        if (!isIndex(r.answer, r.choices.length)) return bad("answer must be a 0-based index into choices");
        return { kind: "predict", ...common, code: r.code as string | undefined, choices: r.choices, answer: r.answer };
      }
      if (!Array.isArray(r.trace) || r.trace.length < 1 || r.trace.length > 20) return bad("trace must be 1-20 steps");
      const trace = r.trace.map((s: unknown, i: number) => {
        const t = s as { label?: unknown; choices?: unknown; answer?: unknown } | null;
        if (!t || !isStr(t.label) || !strList(t.choices) || t.choices.length < 2 || t.choices.length > 8 || !isIndex(t.answer, t.choices.length)) return bad(`trace step ${i} needs label, 2-8 choices and a 0-based answer`);
        return { label: t.label as string, choices: t.choices as string[], answer: t.answer as number };
      });
      return { kind: "predict", ...common, code: r.code as string | undefined, trace };
    }
    case "choice": {
      if (r.code !== undefined && (!isStr(r.code) || r.code.length > MAX_TEXT)) return bad("code must be a string");
      if (!strList(r.choices) || r.choices.length < 2 || r.choices.length > 8 || !lengthOk(r.choices)) return bad("choices must be 2-8 strings");
      if (new Set(r.choices).size !== r.choices.length) return bad("choices must be distinct");
      const choices = r.choices as string[];
      const answers = typeof r.answer === "number" ? [r.answer] : r.answer;
      if (!Array.isArray(answers) || answers.length < 1 || new Set(answers).size !== answers.length || !answers.every((a) => isIndex(a, choices.length))) return bad("answer must be a 0-based index, or an array of distinct indices for select-all");
      if (answers.length === choices.length) return bad("selecting every choice is not a question");
      return { kind: "choice", ...common, code: r.code as string | undefined, choices, answers: [...answers].sort((a, b) => a - b) };
    }
    case "typed": {
      if (r.code !== undefined && (!isStr(r.code) || r.code.length > MAX_TEXT)) return bad("code must be a string");
      if (!strList(r.accepted) || r.accepted.length < 1 || r.accepted.length > 20 || r.accepted.some((a) => !a.trim() || a.length > 120)) return bad("accepted must be 1-20 short non-blank strings");
      if (r.caseSensitive !== undefined && typeof r.caseSensitive !== "boolean") return bad("caseSensitive must be a boolean");
      return { kind: "typed", ...common, code: r.code as string | undefined, accepted: r.accepted, caseSensitive: r.caseSensitive as boolean | undefined };
    }
    case "match": {
      if (!Array.isArray(r.pairs) || r.pairs.length < 2 || r.pairs.length > 8) return bad("pairs must be 2-8 items");
      const pairs = r.pairs.map((x: unknown, i: number) => {
        const q = x as { left?: unknown; right?: unknown } | null;
        if (!q || !isStr(q.left) || !isStr(q.right) || !q.left.trim() || !q.right.trim() || q.left.length > 200 || q.right.length > 200) return bad(`pair ${i} needs non-blank left and right strings`);
        return { left: q.left as string, right: q.right as string };
      });
      if (new Set(pairs.map((x) => x.right)).size !== pairs.length || new Set(pairs.map((x) => x.left)).size !== pairs.length) return bad("lefts and rights must each be distinct");
      return { kind: "match", ...common, pairs };
    }
    default: {
      if (!isStr(r.code) || !r.code.trim() || r.code.length > MAX_TEXT) return bad("code must be a non-empty string");
      const n = r.code.split("\n").length;
      if (!Number.isInteger(r.bugLine) || (r.bugLine as number) < 1 || (r.bugLine as number) > n) return bad(`bugLine must be a 1-based line number within the ${n} lines of code`);
      if (!isStr(r.explain)) return bad("bug practice needs explain, shown after the right line is found");
      return { kind: "bug", ...common, code: r.code, bugLine: r.bugLine as number };
    }
  }
}

// ---------- Public view ----------

function hash(s: string) {
  let h = 2166136261;
  for (let i = 0; i < s.length; i++) h = Math.imul(h ^ s.charCodeAt(i), 16777619);
  return h >>> 0;
}

/** Stable shuffle seeded by the practice id (same order on every load), never equal to the input order when it can differ. */
export function shuffled<T>(xs: readonly T[], seed: string): T[] {
  const same = (a: readonly T[]) => a.every((x, i) => x === xs[i]);
  let state = hash(seed) || 1;
  const rand = () => ((state = Math.imul(state ^ (state >>> 15), 2246822507) ^ Math.imul(state ^ (state >>> 13), 3266489909) ^ 0x9e3779b9), (state >>> 0) / 4294967296);
  const distinct = new Set(xs).size > 1;
  let out = xs.slice();
  for (let attempt = 0; attempt < 8; attempt++) {
    out = xs.slice();
    for (let i = out.length - 1; i > 0; i--) { const j = Math.floor(rand() * (i + 1)); [out[i], out[j]] = [out[j], out[i]]; }
    if (!distinct || !same(out)) break;
  }
  return out;
}

export function toPublicPractice(p: Practice, lessonExerciseId: string, difficulty: Difficulty, idOverride?: string): PublicPractice {
  const id = idOverride ?? practiceIdOf(lessonExerciseId);
  const head = { id, prompt: p.prompt, xp: practiceXpFor(difficulty), fullXp: xpFor(difficulty) };
  switch (p.kind) {
    case "parsons":
      return { ...head, kind: "parsons", lines: shuffled([...p.lines, ...p.distractors], id), hasDistractors: p.distractors.length > 0 };
    case "blanks":
      return { ...head, kind: "blanks", template: p.template, tokens: shuffled(p.tokens, id), slots: slotCount(p.template) };
    case "predict":
      return { ...head, kind: "predict", ...(p.code !== undefined ? { code: p.code } : {}), ...(p.choices ? { choices: p.choices } : {}), ...(p.trace ? { trace: p.trace.map(({ label, choices }) => ({ label, choices })) } : {}) };
    case "bug":
      return { ...head, kind: "bug", code: p.code };
    case "choice":
      return { ...head, kind: "choice", ...(p.code !== undefined ? { code: p.code } : {}), choices: p.choices, multi: p.answers.length > 1 };
    case "typed":
      return { ...head, kind: "typed", ...(p.code !== undefined ? { code: p.code } : {}) };
    case "match":
      return { ...head, kind: "match", lefts: p.pairs.map((x) => x.left), rights: shuffled(p.pairs.map((x) => x.right), id) };
  }
}

// ---------- Checking ----------

export type PracticeCheck =
  | { valid: false; error: string }
  | {
      valid: true;
      /** The submission is the canonical answer. */
      exact: boolean;
      /** Source assembled from a parsons/blanks submission, for running against the lesson's tests. */
      code?: string;
      /** Whether a non-canonical submission may still be accepted by running `code` against the lesson's hidden tests. */
      testable: boolean;
      feedback?: string;
      /** For answers that can be partly right: how many parts were right. Absent for single-answer kinds. */
      partial?: { right: number; total: number };
    };

const sameList = (a: readonly unknown[], b: readonly unknown[]) => a.length === b.length && a.every((x, i) => x === b[i]);
function subMultiset(sub: string[], pool: string[]) {
  const left = [...pool];
  for (const s of sub) { const i = left.indexOf(s); if (i < 0) return false; left.splice(i, 1); }
  return true;
}

export const assembleParsons = (p: ParsonsPractice, lines: string[]) => {
  const body = lines.join("\n");
  return (p.base ? p.base.replace("{{code}}", () => body) : body).replace(/\n*$/, "\n");
};
export const fillBlanks = (p: BlanksPractice, values: string[]) => p.template.replace(SLOT, (_, n) => values[Number(n)] ?? "");

/** Pure answer check. Never throws on hostile input. */
export function checkPractice(p: Practice, submission: unknown): PracticeCheck {
  const invalid = (error: string): PracticeCheck => ({ valid: false, error });
  switch (p.kind) {
    case "parsons": {
      if (!strList(submission) || submission.length === 0) return invalid("Submit the lines you chose, in order.");
      if (submission.length > p.lines.length + p.distractors.length || !subMultiset(submission, [...p.lines, ...p.distractors])) return invalid("Those lines are not part of this exercise.");
      const exact = sameList(submission, p.lines);
      const placed = submission.filter((l, i) => l === p.lines[i]).length;
      return { valid: true, exact, code: assembleParsons(p, submission), testable: p.runTests !== false, partial: { right: placed, total: p.lines.length }, feedback: exact ? undefined : `${placed} of ${p.lines.length} lines are in the right place.` };
    }
    case "blanks": {
      const n = p.answers.length;
      if (!strList(submission) || submission.length !== n) return invalid(`Fill all ${n} blanks.`);
      if (!subMultiset(submission, p.tokens)) return invalid("Those pieces are not part of this exercise.");
      const exact = sameList(submission, p.answers);
      const right = submission.filter((v, i) => v === p.answers[i]).length;
      return { valid: true, exact, code: fillBlanks(p, submission), testable: p.runTests !== false, partial: { right, total: n }, feedback: exact ? undefined : `${right} of ${n} blanks are right.` };
    }
    case "predict": {
      if (p.trace) {
        if (!Array.isArray(submission) || submission.length !== p.trace.length || !submission.every((v, i) => isIndex(v, p.trace![i].choices.length))) return invalid("Answer every step.");
        const right = (submission as number[]).filter((v, i) => v === p.trace![i].answer).length;
        const exact = right === p.trace.length;
        return { valid: true, exact, testable: false, partial: { right, total: p.trace.length }, feedback: exact ? undefined : `${right} of ${p.trace.length} steps are right.` };
      }
      if (!isIndex(submission, p.choices!.length)) return invalid("Pick one of the choices.");
      return { valid: true, exact: submission === p.answer, testable: false, feedback: submission === p.answer ? undefined : "Not quite. Trace it again." };
    }
    case "bug": {
      const n = p.code.split("\n").length;
      if (!Number.isInteger(submission) || (submission as number) < 1 || (submission as number) > n) return invalid("Pick a line of the code.");
      return { valid: true, exact: submission === p.bugLine, testable: false, feedback: submission === p.bugLine ? undefined : "That line is fine. Look again." };
    }
    case "choice": {
      if (!Array.isArray(submission) || submission.length < 1 || new Set(submission).size !== submission.length || !submission.every((v) => isIndex(v, p.choices.length))) return invalid("Pick at least one choice.");
      const exact = sameList([...(submission as number[])].sort((a, b) => a - b), p.answers);
      const hint = p.answers.length > 1 ? "Not quite. Some correct choices are missing or some chosen ones are wrong." : "Not quite. Think it through again.";
      return { valid: true, exact, testable: false, feedback: exact ? undefined : hint };
    }
    case "typed": {
      if (!isStr(submission) || !submission.trim() || submission.length > 200) return invalid("Type your answer.");
      const norm = (v: string) => { const t = v.trim().replace(/\s+/g, " "); return p.caseSensitive ? t : t.toLowerCase(); };
      const exact = p.accepted.some((a) => norm(a) === norm(submission));
      return { valid: true, exact, testable: false, feedback: exact ? undefined : "That is not the answer we were looking for." };
    }
    case "match": {
      const n = p.pairs.length;
      if (!strList(submission) || submission.length !== n) return invalid(`Match all ${n} items.`);
      if (!subMultiset(submission, p.pairs.map((x) => x.right))) return invalid("Those matches are not part of this exercise.");
      const right = submission.filter((v, i) => v === p.pairs[i].right).length;
      return { valid: true, exact: right === n, testable: false, partial: { right, total: n }, feedback: right === n ? undefined : `${right} of ${n} matches are right.` };
    }
  }
}

/** The submission that is exactly right. For tests and the content verifier. */
export function canonicalSubmission(p: Practice): PracticeSubmission {
  switch (p.kind) {
    case "parsons": return p.lines;
    case "blanks": return p.answers;
    case "bug": return p.bugLine;
    case "predict": return p.trace ? p.trace.map((t) => t.answer) : p.answer!;
    case "choice": return p.answers;
    case "typed": return p.accepted[0];
    case "match": return p.pairs.map((x) => x.right);
  }
}

/** Plain-text correct answer, shown after a review card is answered. Only ever sent once the schedule has moved. */
export function revealAnswer(p: Practice): string {
  switch (p.kind) {
    case "parsons": return p.lines.join("\n");
    case "blanks": return fillBlanks(p, p.answers);
    case "predict": return p.trace ? p.trace.map((t) => `${t.label}: ${t.choices[t.answer]}`).join("\n") : p.choices![p.answer!];
    case "bug": return `Line ${p.bugLine}: ${p.code.split("\n")[p.bugLine - 1]?.trim() ?? ""}`;
    case "choice": return p.answers.map((i) => p.choices[i]).join("\n");
    case "typed": return p.accepted[0];
    case "match": return p.pairs.map((x) => `${x.left} → ${x.right}`).join("\n");
  }
}
