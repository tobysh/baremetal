// Pre-written explanations for common failures. Runs in the browser, costs
// nothing, and covers most of what beginners hit; the "Elaborate" button hands
// the specific case to the AI tutor.

import type { Language } from "./curriculum/types";

export type Explanation = { id: string; title: string; body: string };

type TestCase = { name: string; passed: boolean; message?: string };
export type FailureInput = {
  language: Language;
  mode: "stdout" | "harness";
  code: string;
  starter: string;
  compileFailed: boolean;
  compileOutput: string;
  error: string | null;
  results: TestCase[];
  stderr?: string;
};

type Rule = { id: string; test: (text: string) => boolean; title: string; body: string };

const C_COMPILE: Rule[] = [
  {
    id: "c-semicolon",
    test: (t) => /expected ';'/.test(t),
    title: "A semicolon is missing",
    body: "In C every statement ends with `;`. The compiler usually reports the error on the line *after* the one missing it, so look at the end of the previous line.",
  },
  {
    id: "c-undeclared",
    test: (t) => /undeclared|was not declared/.test(t),
    title: "You used a name C doesn't know",
    body: "A variable must be declared (e.g. `int count = 0;`) before it's used, and names are case-sensitive: `Count` and `count` are different. Check spelling, and that the variable was declared inside the same `{ }` block or an enclosing one.",
  },
  {
    id: "c-implicit",
    test: (t) => /implicit declaration of function/.test(t),
    title: "A function is used before C knows about it",
    body: "Either the name is misspelled, or you need the header that declares it — `strlen` needs `#include <string.h>`, `malloc` needs `#include <stdlib.h>`, `printf` needs `#include <stdio.h>`.",
  },
  {
    id: "c-pointer-int",
    test: (t) => /makes pointer from integer|makes integer from pointer|incompatible pointer|incompatible integer to pointer/.test(t),
    title: "A pointer and a value are mixed up",
    body: "`p` is an address; `*p` is the value stored there; `&x` is the address of `x`. This error means you gave an address where a value was expected, or the other way round. Check each `*` and `&` on that line.",
  },
  {
    id: "c-conflicting",
    test: (t) => /conflicting types|too (few|many) arguments/.test(t),
    title: "A function's signature doesn't match",
    body: "The tests call your function with the exact signature in the task. If you changed the return type or parameters, change them back — or check that every call passes the right number of arguments.",
  },
  {
    id: "c-undefined-ref",
    test: (t) => /undefined reference to `(?!main)/.test(t),
    title: "A function the tests need doesn't exist",
    body: "The tests call a function by a specific name. Make sure it's spelled exactly as in the task (including underscores) and isn't inside another function.",
  },
  {
    id: "c-brace",
    test: (t) => /expected declaration or statement at end of input|expected '}'/.test(t),
    title: "A `{` isn't closed",
    body: "Every `{` needs a matching `}`. Count them in the function the error points to — the editor highlights the matching bracket when your cursor is next to one.",
  },
];

const C_WARN: Rule[] = [
  {
    id: "c-no-return",
    test: (t) => /control reaches end of non-void function/.test(t),
    title: "Some paths don't return a value",
    body: "Your function promises to return a value, but at least one way through it reaches the closing `}` without a `return`. The caller then gets garbage. Make sure every branch returns.",
  },
  {
    id: "c-unused-param",
    test: (t) => /unused parameter/.test(t),
    title: "A parameter is never used",
    body: "That usually means the function body still doesn't do anything with its input — often the starter stub hasn't been filled in yet.",
  },
  {
    id: "c-assign-in-if",
    test: (t) => /suggest parentheses around assignment used as truth value/.test(t),
    title: "`=` used where `==` was meant",
    body: "`if (x = 5)` *assigns* 5 to `x` and is always true. To compare, use `==`.",
  },
];

const ASM_COMPILE: Rule[] = [
  {
    id: "asm-unknown-insn",
    test: (t) => /no such instruction|unknown mnemonic/.test(t),
    title: "The assembler doesn't know that instruction",
    body: "Check the spelling of the mnemonic (`mov`, `add`, `lea`, `imul`...). Also make sure the file starts with `.intel_syntax noprefix`: without it the assembler expects AT&T syntax, where operands are reversed and registers need a `%`.",
  },
  {
    id: "asm-operands",
    test: (t) => /operand type mismatch|invalid (instruction )?operands?|too many memory references|number of operands mismatch|ambiguous operand size|no instruction mnemonic suffix given/.test(t),
    title: "The operands don't fit that instruction",
    body: "x86 can't move memory to memory, and both operands must be the same size (`rax` is 64-bit, `eax` is 32-bit, `al` is 8-bit). When one side is memory with no register, say the size: `mov DWORD PTR [rdi], 5`.",
  },
  {
    id: "asm-bad-register",
    test: (t) => /bad register name|unsupported instruction/.test(t),
    title: "That register name isn't valid",
    body: "In Intel syntax registers are written bare (`rax`, `rdi`), with no `%`. 64-bit names start with `r`: `rax rbx rcx rdx rsi rdi rbp rsp r8`-`r15`.",
  },
  {
    id: "asm-symbol",
    test: (t) => /undefined reference to `(?!main)|symbol .* is already defined/.test(t),
    title: "A label or function name is wrong",
    body: "The tests look for your function by name, so declare it with `.globl name` and a matching `name:` label, spelled exactly as in the task. A label can only be defined once, and each line holds just one instruction.",
  },
  {
    id: "asm-directive",
    test: (t) => /unknown pseudo-op|junk at end of line|expected comma/.test(t),
    title: "A directive or line is malformed",
    body: "Directives start with a dot (`.text`, `.globl`). Comments start with `#` or `;`. Operands are separated by commas, e.g. `add rax, rsi`.",
  },
];

const ASM_RUNTIME: Rule[] = [
  {
    id: "asm-crash",
    test: (t) => /Segmentation fault|Bus error/.test(t),
    title: "Your assembly touched memory it doesn't own",
    body: "In assembly a crash usually means a register holding a value was used as an address (`[rdi]`), or you forgot `ret` and the CPU ran off the end of your function, or `push`/`pop` didn't balance so `ret` jumped to a garbage address. Check each `[ ]` and that every `push` has a `pop` before `ret`.",
  },
  {
    id: "asm-fpe",
    test: (t) => /Arithmetic error/.test(t),
    title: "A division went wrong",
    body: "`idiv` divides `rdx:rax` by its operand: put the dividend in `rax`, sign-extend it into `rdx` with `cqo`, and never divide by zero. The quotient ends up in `rax` and the remainder in `rdx`.",
  },
];

const RUST_COMPILE: Rule[] = [
  {
    id: "rs-moved",
    test: (t) => /E0382|use of moved value|borrow of moved value/.test(t),
    title: "A value was used after it was moved",
    body: "Passing a `String` or `Vec` by value *moves* it: the old variable can't be used afterwards. Borrow it instead with `&value` (or `&mut value` to change it), or `.clone()` it if you really need two copies.",
  },
  {
    id: "rs-borrow",
    test: (t) => /E0502|E0499|cannot borrow .* as mutable because it is also borrowed/.test(t),
    title: "Borrowing rules were broken",
    body: "Rust allows either many readers (`&T`) or one writer (`&mut T`) at a time, never both. Finish using the shared borrow before you start mutating, or restructure so the borrows don't overlap.",
  },
  {
    id: "rs-mismatch",
    test: (t) => /E0308|mismatched types/.test(t),
    title: "The types don't match",
    body: "Read the `expected X, found Y` part of the error. Common causes: returning `&str` where `String` is expected (use `.to_string()`), integer types differing (`i32` vs `usize` — convert with `as`), or a trailing `;` turning the last expression into `()`.",
  },
  {
    id: "rs-unknown",
    test: (t) => /E0425|E0433|cannot find (value|function|type)/.test(t),
    title: "A name isn't in scope",
    body: "Check spelling, and whether you need a `use` line — e.g. `use std::collections::HashMap;`.",
  },
  {
    id: "rs-immutable",
    test: (t) => /E0384|E0596|cannot assign twice to immutable|cannot borrow .* as mutable, as it is not declared as mutable/.test(t),
    title: "The variable isn't `mut`",
    body: "Variables in Rust are immutable by default. Declare it with `let mut name = ...;` if you need to change it.",
  },
  {
    id: "rs-lifetime",
    test: (t) => /E0106|missing lifetime specifier/.test(t),
    title: "Rust needs to know how long a reference lives",
    body: "When a function returns a reference, Rust must know which input it borrows from. Often the fix is to return an owned `String` instead, or to tie them with a lifetime like `fn f<'a>(s: &'a str) -> &'a str`.",
  },
  {
    id: "rs-method",
    test: (t) => /E0599|no method named/.test(t),
    title: "That method doesn't exist on this type",
    body: "Check the type you're calling it on — e.g. `.len()` exists on `String` and `Vec`, but you may need `.chars()` first to work with characters, or `.iter()` before `.map()`.",
  },
];

const RUNTIME: Rule[] = [
  {
    id: "segfault",
    test: (t) => /Segmentation fault/.test(t),
    title: "The program read or wrote memory it doesn't own",
    body: "Usual causes: dereferencing a `NULL` or uninitialised pointer, indexing past the end of an array (remember the last index is `len - 1`), or using memory after `free`. Find the test that crashed and trace your pointers by hand for that input.",
  },
  {
    id: "ub-null",
    test: (t) => /null pointer/.test(t),
    title: "A NULL pointer was used",
    body: "Something that should point to real memory is `NULL` — often a `malloc` that wasn't checked, or a list node that's empty. Check for `NULL` before using `->` or `*`.",
  },
  {
    id: "ub-overflow",
    test: (t) => /signed integer overflow|attempt to (add|subtract|multiply) with overflow/.test(t),
    title: "A number got too big (or too small) for its type",
    body: "An `int` holds about ±2.1 billion; going past that is an overflow. Use a wider type (`long`, `i64`), or check your loop — `size_t` and `usize` can't go below 0, so `i - 1` when `i` is 0 wraps around.",
  },
  {
    id: "ub-bounds",
    test: (t) => /index out of bounds|out of bounds/.test(t),
    title: "An index went past the end",
    body: "Valid indices are `0` to `len - 1`. Check loop conditions (`<` vs `<=`) and the empty case, where there are no valid indices at all.",
  },
  {
    id: "timeout",
    test: (t) => /Timed out|ran for more than|never ends/.test(t),
    title: "The program never finished",
    body: "Most likely a loop whose condition never becomes false — check that the loop variable actually changes each time round. In stdout exercises, a `scanf` waiting for input that isn't there also causes this.",
  },
  {
    id: "abort",
    test: (t) => /Aborted|double free|free\(\): invalid/.test(t),
    title: "The C library stopped the program",
    body: "Often caused by freeing the same pointer twice, or freeing something that wasn't from `malloc`. After `free(p)`, don't use `p` again.",
  },
  {
    id: "rs-unwrap",
    test: (t) => /called `(Option|Result)::unwrap\(\)` on/.test(t),
    title: "`unwrap()` hit a None or Err",
    body: "`unwrap()` panics if there's no value. Handle the other case with `match`, `if let`, or `?` instead of assuming it always succeeds.",
  },
];

const match = (rules: Rule[], text: string) => rules.filter((r) => r.test(text)).map(({ id, title, body }) => ({ id, title, body }));

export function explainFailure(f: FailureInput): Explanation[] {
  const out: Explanation[] = [];
  const unchanged = f.code.trim() === f.starter.trim();

  if (f.compileFailed) {
    out.push(...match(f.language === "c" ? C_COMPILE : f.language === "asm" ? ASM_COMPILE : RUST_COMPILE, f.compileOutput));
    if (!out.length)
      out.push({
        id: "compile-generic",
        title: "The code doesn't compile yet",
        body: "Read the *first* error only: it names a file, line and column (like `main.c:7:5`, or `starter.s:7`). Fix that one and run again — later errors are often knock-on effects of the first.",
      });
    return out.slice(0, 3);
  }

  const runtimeText = [f.error ?? "", f.stderr ?? "", ...f.results.map((r) => r.message ?? "")].join("\n");
  if (f.language === "asm") out.push(...match(ASM_RUNTIME, runtimeText));
  out.push(...match(f.language === "asm" ? RUNTIME.filter((r) => r.id !== "segfault" && r.id !== "ub-null" && r.id !== "abort") : RUNTIME, runtimeText));

  const failed = f.results.filter((r) => !r.passed);
  const passed = f.results.length - failed.length;
  if (failed.length) {
    if (unchanged) {
      out.push({
        id: "unchanged",
        title: "You haven't changed the starter code yet",
        body: "The starter is just a skeleton that compiles. Read the task, then replace the placeholder body of each function with your own code.",
      });
    } else if (passed === 0 && failed.every((r) => /got (0|""|''|\(nothing\)|\bfalse\b)|left: (0|""|false)/.test(r.message ?? ""))) {
      out.push({
        id: "placeholder",
        title: "The function still returns its placeholder",
        body: "Every test got back `0` or an empty value — the same thing the starter returns. Check that your function actually `return`s the result it computes, and that the work happens before the `return`.",
      });
    } else if (passed > 0) {
      const edge = failed.find((r) => /empty|zero|0\b|negative|single|one\b|""|odd|even|null/i.test(r.name));
      out.push(
        edge
          ? {
              id: "edge",
              title: "Most cases work — an edge case doesn't",
              body: `Your idea is right for typical inputs. The failing case ("${edge.name}") is an edge case: try it by hand, step by step, with the smallest input. Empty strings, a single element, zero and negative numbers are where loops and indices usually go wrong.`,
            }
          : {
              id: "partial",
              title: "Some cases work, some don't",
              body: "Compare a passing test with a failing one. What's different about the failing input? Trace your code by hand with it and write down each variable's value as you go.",
            },
      );
    }

    if (f.mode === "stdout") {
      const msg = failed[0]?.message ?? "";
      const exp = msg.match(/Expected output:\n([\s\S]*?)\n\nYour output:/)?.[1] ?? "";
      const got = msg.match(/Your output:\n([\s\S]*)$/)?.[1] ?? "";
      if (exp && got) {
        if (exp.toLowerCase() === got.toLowerCase())
          out.push({ id: "case", title: "Capital letters differ", body: "The output must match exactly, including upper and lower case." });
        else if (exp.replace(/\s+/g, "") === got.replace(/\s+/g, ""))
          out.push({
            id: "whitespace",
            title: "Spacing or line breaks differ",
            body: "The text is right but spaces or newlines aren't. Remember `\\n` at the end of each line, and check for missing or extra spaces around `=`, `:` and commas.",
          });
        else if (got.trim() && exp.startsWith(got.trim()))
          out.push({ id: "short", title: "The output stops early", body: "The first part matches but some lines are missing. Check your loop runs the right number of times." });
      }
    }
  }

  if (!out.length && (failed.length || f.error))
    out.push({
      id: "generic",
      title: "Something doesn't match yet",
      body: "Pick one failing test, work out by hand what your code does for that input, and compare it with what the test expected.",
    });

  // Warnings are only worth pointing out once there's a problem to explain.
  if (failed.length || f.error) out.push(...match(f.language === "c" ? C_WARN : [], f.compileOutput));
  return out.slice(0, 3);
}
