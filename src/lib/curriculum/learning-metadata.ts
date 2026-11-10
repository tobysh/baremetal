/** Authored teaching intent, not evidence that a learner has mastered a concept. */
export type LessonLearning = {
  objectives?: string[];
  prerequisites?: string[];
  concepts?: string[];
};

const FIELDS = ["objectives", "prerequisites", "concepts"] as const;
const CONCEPT_ID = /^[a-z][a-z0-9]*(?:-[a-z0-9]+)*(?:\.[a-z][a-z0-9]*(?:-[a-z0-9]+)*)+$/;

/** Validate optional learning.json without changing legacy frontmatter semantics. */
export function parseLearningMetadata(raw: unknown, where = "learning.json"): LessonLearning {
  const fail = (message: string): never => { throw new Error(`${where}: ${message}`); };
  if (!raw || typeof raw !== "object" || Array.isArray(raw)) fail("must be an object");
  const record = raw as Record<string, unknown>;
  const keys = Object.keys(record);
  if (!keys.length) fail("must contain at least one of objectives, prerequisites or concepts");
  for (const key of keys) if (!FIELDS.some((field) => field === key)) fail(`unknown field "${key}"`);
  const parsed: LessonLearning = {};
  for (const key of FIELDS) {
    if (!(key in record)) continue;
    const value = record[key];
    if (!Array.isArray(value) || value.length === 0) fail(`${key} must be a non-empty array of strings`);
    const seen = new Set<string>();
    parsed[key] = (value as unknown[]).map((item, index) => {
      if (typeof item !== "string" || !item.trim()) fail(`${key}[${index}] must be a non-blank string`);
      const text = (item as string).trim();
      if (seen.has(text)) fail(`${key} contains duplicate "${text}"`);
      if (key === "concepts" && !CONCEPT_ID.test(text)) fail(`concept "${text}" must be a lowercase namespaced ID, such as c.pointer-addresses`);
      seen.add(text);
      return text;
    });
  }
  return parsed;
}
