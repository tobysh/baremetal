import type { Language } from "@/lib/curriculum/types";

/** Tap-to-insert pieces for phones. Templates use CodeMirror snippet syntax: `${1:name}` tab stops, `\t` = one indent. */
export type Snippet = { label: string; template: string };

/** Single-token keys for the symbol bar, shared by every language first and then by language. */
export const COMMON_SYMBOLS = ["{", "}", "(", ")", "[", "]", ";", '"', "'"] as const;

export const LANGUAGE_SYMBOLS: Record<Language, readonly string[]> = {
  c: ["*", "&", "->", "#", "<", ">", "=", "!", "%", "\\n"],
  kernel: ["*", "&", "->", "#", "<", ">", "=", "|", "0x", "\\n"],
  rust: ["::", "->", "&", "mut ", "'", "!", "|", "<", ">", "="],
  go: [":=", "&", "*", ".", ",", "<-", "!", "=", "<", ">"],
  asm: [",", ":", "+", "-", "*", ".", "#", "0x", "rax", "rdi", "rsi"],
};

const C_SNIPPETS: Snippet[] = [
  { label: "for", template: "for (int ${1:i} = 0; ${1:i} < ${2:n}; ${1:i}++) {\n\t${3}\n}" },
  { label: "if", template: "if (${1:condition}) {\n\t${2}\n}" },
  { label: "while", template: "while (${1:condition}) {\n\t${2}\n}" },
  { label: "printf", template: 'printf("${1:%d}\\n", ${2:value});' },
  { label: "func", template: "${1:int} ${2:name}(${3:int a}) {\n\t${4}\n}" },
  { label: "main", template: "int main(void) {\n\t${1}\n\treturn 0;\n}" },
];

const RUST_SNIPPETS: Snippet[] = [
  { label: "fn", template: "fn ${1:name}(${2:a: i32}) -> ${3:i32} {\n\t${4}\n}" },
  { label: "fn main", template: "fn main() {\n\t${1}\n}" },
  { label: "for", template: "for ${1:i} in ${2:0..10} {\n\t${3}\n}" },
  { label: "if", template: "if ${1:condition} {\n\t${2}\n}" },
  { label: "match", template: "match ${1:value} {\n\t${2:pattern} => ${3:result},\n\t_ => ${4:default},\n}" },
  { label: "println!", template: 'println!("${1:{}}", ${2:value});' },
  { label: "let", template: "let ${1:name} = ${2:value};" },
];

const GO_SNIPPETS: Snippet[] = [
  { label: "func", template: "func ${1:name}(${2:a int}) ${3:int} {\n\t${4}\n}" },
  { label: "main", template: "func main() {\n\t${1}\n}" },
  { label: "for", template: "for ${1:i} := 0; ${1:i} < ${2:n}; ${1:i}++ {\n\t${3}\n}" },
  { label: "range", template: "for ${1:i}, ${2:v} := range ${3:items} {\n\t${4}\n}" },
  { label: "if", template: "if ${1:condition} {\n\t${2}\n}" },
  { label: "Println", template: "fmt.Println(${1:value})" },
  { label: "err", template: "if err != nil {\n\treturn ${1:err}\n}" },
];

const ASM_SNIPPETS: Snippet[] = [
  { label: "mov", template: "mov ${1:rax}, ${2:0}" },
  { label: "add", template: "add ${1:rax}, ${2:rbx}" },
  { label: "cmp", template: "cmp ${1:rax}, ${2:0}" },
  { label: "jmp", template: "j${1:e} ${2:label}" },
  { label: "label", template: "${1:loop}:\n\t${2}" },
  { label: "call", template: "call ${1:function}" },
  { label: "ret", template: "ret" },
];

export const LANGUAGE_SNIPPETS: Record<Language, Snippet[]> = {
  c: C_SNIPPETS,
  kernel: C_SNIPPETS,
  rust: RUST_SNIPPETS,
  go: GO_SNIPPETS,
  asm: ASM_SNIPPETS,
};
