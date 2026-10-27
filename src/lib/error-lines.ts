import { LANGUAGE_FILE, type Language } from "./curriculum/types";

/**
 * Line numbers in the learner's file that a compile error, panic or sanitizer
 * report points at, so the editor can highlight them. Warnings are ignored.
 */
export function errorLines(language: Language, compileOutput: string, runtimeOutput: string, compileFailed: boolean): number[] {
  const file = LANGUAGE_FILE[language].replace(".", "\\.");
  const lines = new Set<number>();
  let severity = "";
  for (const line of compileOutput.split("\n")) {
    const header = line.match(/^(error|warning)\b/);
    if (header) severity = header[1];
    const rustc = line.match(new RegExp(`-->\\s+(?:\\S*/)?${file}:(\\d+)`));
    if (rustc) {
      if (severity === "error") lines.add(Number(rustc[1]));
      continue;
    }
    const located = line.match(new RegExp(`(?:^|/)${file}:(\\d+)(?::\\d+)?:\\s*(.*)`));
    if (located && (language === "go" ? compileFailed : /error/i.test(located[2]))) lines.add(Number(located[1]));
  }
  for (const line of runtimeOutput.split("\n")) {
    const found = line.match(new RegExp(`(?:^|/)${file}:(\\d+):\\d+: runtime error`)) ?? line.match(new RegExp(`panicked at (?:\\S*/)?${file}:(\\d+)`));
    if (found) lines.add(Number(found[1]));
  }
  return [...lines].sort((a, b) => a - b);
}
