// Pure helpers that make equivalent tutor questions share one cache entry.
// Kept free of server-only imports so scripts can test them directly.

/** Collapses whitespace and drops comments outside string literals, so reformatted or re-commented code maps to the same key. */
export function normalizeCode(code: string, language: string): string {
  const src = code.replace(/\r\n?/g, "\n");
  const asm = language === "asm";
  const quotes = language === "rust" || asm ? ['"'] : ['"', "'", ...(language === "go" ? ["`"] : [])];
  let out = "";
  let i = 0;
  while (i < src.length) {
    const c = src[i];
    const next = src[i + 1];
    if (c === "/" && next === "*") {
      const end = src.indexOf("*/", i + 2);
      i = end < 0 ? src.length : end + 2;
      out += " ";
    } else if ((c === "/" && next === "/") || (asm && c === "#")) {
      while (i < src.length && src[i] !== "\n") i++;
    } else if (quotes.includes(c)) {
      let j = i + 1;
      while (j < src.length && src[j] !== c && (src[j] !== "\n" || c === "`")) j += src[j] === "\\" && c !== "`" ? 2 : 1;
      out += src.slice(i, j + 1); // literal kept verbatim
      i = j + 1;
    } else if (/\s/.test(c)) {
      let j = i;
      let newline = false;
      while (j < src.length && /\s/.test(src[j])) if (src[j++] === "\n") newline = true;
      const prev = out.at(-1) ?? "";
      const after = src[j] ?? "";
      // Spaces only matter between two word characters (or two operator characters, as in "a - -b").
      const needed = (/\w/.test(prev) && /\w/.test(after)) || (/[-+&|<>=*/]/.test(prev) && /[-+&|<>=*/]/.test(after));
      out += newline ? "\n" : needed ? " " : "";
      i = j;
    } else {
      out += c;
      i++;
    }
  }
  // Whitespace runs next to a newline are noise.
  return out.replace(/ ?\n ?/g, "\n").replace(/\n+/g, "\n").trim();
}

/** Trims program output: line endings, colours, trailing spaces and blank-line runs. */
export function normalizeOutput(output: string): string {
  return output
    .replace(/\r\n?/g, "\n")
    .replace(/\u001b\[[0-9;]*[A-Za-z]/g, "")
    .replace(/[ \t]+$/gm, "")
    .replace(/\n{3,}/g, "\n\n")
    .trim();
}

/** Case, spacing and trailing punctuation do not change what a learner is asking. */
export function normalizeMessage(message: string): string {
  return message.toLowerCase().replace(/\s+/g, " ").replace(/[\s.!?]+$/, "").trim();
}
