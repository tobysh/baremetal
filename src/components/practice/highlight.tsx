import type { CSSProperties, ReactNode } from "react";
import type { Language } from "@/lib/curriculum/types";

// A small, forgiving tokenizer for the short snippets shown in practice. It colours with the editor's --code-*
// tokens and never has to be exact: anything it does not recognise is plain text.

const KEYWORDS: Record<Language, string> = {
  c: "if else for while do switch case default break continue return sizeof struct enum union typedef static const extern goto volatile inline",
  kernel: "if else for while do switch case default break continue return sizeof struct enum union typedef static const extern goto volatile inline asm __asm__",
  rust: "fn let mut if else for while loop match return struct enum impl use mod pub const static as in ref move unsafe where trait type self Self break continue",
  go: "func var const if else for range switch case default return struct interface type package import map chan go defer select break continue",
  asm: "section global extern db dw dd dq times equ",
};
const TYPES: Record<Language, string> = {
  c: "int char long short unsigned signed float double void size_t uint8_t uint16_t uint32_t uint64_t int8_t int16_t int32_t int64_t bool FILE NULL",
  kernel: "int char long short unsigned signed float double void size_t uint8_t uint16_t uint32_t uint64_t int8_t int16_t int32_t int64_t bool NULL",
  rust: "i8 i16 i32 i64 i128 isize u8 u16 u32 u64 u128 usize f32 f64 bool char str String Vec Option Result Some None Ok Err",
  go: "int int8 int16 int32 int64 uint uint8 uint16 uint32 uint64 float32 float64 string bool byte rune error nil true false",
  asm: "rax rbx rcx rdx rsi rdi rbp rsp r8 r9 r10 r11 r12 r13 r14 r15 eax ebx ecx edx esi edi ebp esp ax bx cx dx al bl cl dl ah bh ch dh rip",
};

const colour = (name: string): CSSProperties => ({ color: `var(--code-${name})` });
const cache = new Map<Language, RegExp>();

function matcher(lang: Language) {
  let re = cache.get(lang);
  if (!re) {
    const comment = lang === "asm" ? String.raw`;.*|#.*` : String.raw`\/\/.*|\/\*.*?\*\/`;
    const macro = lang === "c" || lang === "kernel" ? String.raw`|(?<macro>^\s*#\s*\w+)` : "";
    re = new RegExp(String.raw`(?<comment>${comment})|(?<string>"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*')${macro}|(?<number>\b0x[0-9a-fA-F]+\b|\b\d+(?:\.\d+)?\b)|(?<word>[A-Za-z_][A-Za-z_0-9]*)`, "gm");
    cache.set(lang, re);
  }
  return re;
}

const sets = new Map<string, Set<string>>();
const setOf = (words: string) => sets.get(words) ?? (sets.set(words, new Set(words.split(" "))), sets.get(words)!);

/** Colours one fragment of source. Keys are unique within the returned array. */
export function highlight(src: string, lang: Language, keyPrefix = ""): ReactNode[] {
  const out: ReactNode[] = [];
  const re = matcher(lang);
  const kw = setOf(KEYWORDS[lang]), ty = setOf(TYPES[lang]);
  let last = 0, n = 0;
  re.lastIndex = 0;
  for (const m of src.matchAll(re)) {
    const g = m.groups ?? {};
    if (m.index > last) out.push(src.slice(last, m.index));
    const text = m[0];
    const key = `${keyPrefix}${n++}`;
    let style: CSSProperties | undefined;
    if (g.comment) style = { ...colour("comment"), fontStyle: "italic" };
    else if (g.string) style = colour("string");
    else if (g.macro) style = colour("macro");
    else if (g.number) style = colour("number");
    else if (g.word) style = kw.has(text) ? colour("keyword") : ty.has(text) ? colour("type") : undefined;
    out.push(style ? <span key={key} style={style}>{text}</span> : text);
    last = m.index + text.length;
  }
  if (last < src.length) out.push(src.slice(last));
  return out;
}
