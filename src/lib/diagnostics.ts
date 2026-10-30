// Pre-written explanations for common failures. Runs in the browser, costs
// nothing, and covers most of what beginners hit; the "Elaborate" button hands
// the specific case to the AI tutor.

import type { Language } from "./curriculum/types";

export type Explanation = { id: string; title: string; body: string; nextStep: string };
type Finding = Omit<Explanation, "nextStep">;

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
  runtimeFailed?: boolean;
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

const GO_COMPILE: Rule[] = [
  {
    id: "go-unused-var",
    test: (t) => /declared and not used/.test(t),
    title: "A variable is declared but never used",
    body: "Go refuses to compile unused local variables. Use the variable, or write `_ = name` / assign to the blank identifier `_` if you really want to ignore a value.",
  },
  {
    id: "go-unused-import",
    test: (t) => /imported and not used/.test(t),
    title: "An import isn't used",
    body: "Go won't build with an unused import. Remove it from the `import` block, or use something from the package.",
  },
  {
    id: "go-undefined",
    test: (t) => /undefined: /.test(t),
    title: "A name isn't defined",
    body: "Check the spelling and capitalisation (`Println`, not `println`, for `fmt`), that the package is imported, and that the variable is in scope: a name declared inside `{ }` is gone after the closing brace.",
  },
  {
    id: "go-mismatch",
    test: (t) => /cannot use .* as .* value|mismatched types|invalid operation: .*(mismatched|cannot convert)/.test(t),
    title: "Types don't match",
    body: "Go never converts between types implicitly: `int` and `int64`, or `float64` and `int`, must be converted by hand: `float64(n)`, `int(x)`.",
  },
  {
    id: "go-return-count",
    test: (t) => /not enough return values|too many return values|assignment mismatch|missing return/.test(t),
    title: "The number of return values is wrong",
    body: "A function that declares `(int, error)` must return both values on every path, and callers must receive both: `v, err := f()`. Every code path needs a `return`.",
  },
  {
    id: "go-short-decl",
    test: (t) => /no new variables on left side of :=|non-name .* on left side of :=/.test(t),
    title: "`:=` needs at least one new variable",
    body: "`:=` declares *and* assigns. To change an existing variable use plain `=`.",
  },
  {
    id: "go-interface",
    test: (t) => /does not implement|missing method|cannot use .* \(.*\) as .* value .*: .*method/.test(t),
    title: "A type doesn't satisfy the interface",
    body: "A type implements an interface when it has every method with exactly the right name and signature. If the methods use a pointer receiver (`func (s *T) ...`), pass `&value`, not `value`.",
  },
  {
    id: "go-syntax",
    test: (t) => /syntax error|unexpected newline|expected '\{'|non-declaration statement/.test(t),
    title: "A syntax error",
    body: "Look at the line reported and the one above it. Common causes: the opening `{` must be on the same line as `func`/`if`/`for`; statements outside a function must be declarations; a missing `)` or `,` (a multi-line list needs a trailing comma).",
  },
];

const GO_RUNTIME: Rule[] = [
  {
    id: "go-nil-pointer",
    test: (t) => /nil pointer dereference/.test(t),
    title: "A nil pointer was used",
    body: "A pointer (or map, or interface) that is `nil` was dereferenced. Check it with `if p == nil` before using it, and make sure it was initialised (`&T{}`, `new(T)`).",
  },
  {
    id: "go-nil-map",
    test: (t) => /assignment to entry in nil map/.test(t),
    title: "You wrote to a nil map",
    body: "`var m map[string]int` is nil and can be read but not written. Create it with `make(map[string]int)` or a literal `map[string]int{}` first.",
  },
  {
    id: "go-index",
    test: (t) => /index out of range|slice bounds out of range/.test(t),
    title: "An index went past the end",
    body: "Valid indices are `0` to `len(s)-1`. Check `<` vs `<=` in loops and the empty-slice case.",
  },
  {
    id: "go-deadlock",
    test: (t) => /all goroutines are asleep/.test(t),
    title: "Deadlock: every goroutine is waiting",
    body: "Something is blocked forever: sending on an unbuffered channel with nobody receiving, receiving from a channel nobody sends on or closes, or a `WaitGroup.Wait` whose `Done` never runs. Make sure each send has a matching receive and that the sender calls `close(ch)` when ranging over it.",
  },
  {
    id: "go-closed-channel",
    test: (t) => /send on closed channel|close of closed channel|close of nil channel/.test(t),
    title: "A channel was closed wrongly",
    body: "Only the sender should close a channel, only once, and never send after closing.",
  },
  {
    id: "go-race",
    test: (t) => /concurrent map (read and map )?write|DATA RACE/.test(t),
    title: "Goroutines touched shared data at the same time",
    body: "Protect shared variables and maps with a `sync.Mutex` (`Lock` / `defer Unlock`), or send the data over a channel instead.",
  },
  {
    id: "go-zero-div",
    test: (t) => /integer divide by zero/.test(t),
    title: "Division by zero",
    body: "Integer division by zero panics in Go. Check the divisor first and return an error.",
  },
  {
    id: "go-panic",
    test: (t) => /^panic: /m.test(t),
    title: "The program panicked",
    body: "A panic is Go's crash. Read the line after `panic:` for the reason and the first `main.go:NN` frame for where it happened.",
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

// Bare-metal kernels run in a virtual machine, so a crash is a CPU exception or a reset rather than a signal.
const KERNEL_RUNTIME: Rule[] = [
  {
    id: "kernel-triple-fault",
    test: (t) => /triple fault/i.test(t),
    title: "The CPU gave up: a triple fault",
    body: "An exception happened, then another while the CPU tried to handle it, so the machine reset. On x86 this is nearly always a bad table: a descriptor with a wrong bit, an IDT entry pointing nowhere, a page table that doesn't map the code or stack, or a stack pointer that isn't set up. Check the last structure you built against its layout in the lesson.",
  },
  {
    id: "kernel-page-fault",
    test: (t) => /page fault/i.test(t),
    title: "A page fault: the CPU touched an address that isn't mapped",
    body: "The message gives the address being accessed. If it's tiny (like `0x0` or `0x8`) you followed a null pointer. If it's huge or looks like text, a pointer was overwritten or computed wrongly. If you are writing page tables, check the present bit and that you used the *physical* address of the next table.",
  },
  {
    id: "kernel-gpf",
    test: (t) => /general protection/i.test(t),
    title: "A general protection fault",
    body: "The CPU refused an operation: usually a bad segment selector, a non-canonical address (bits 63:48 must copy bit 47), or an unaligned operand where alignment is required. Look at what you loaded just before the fault.",
  },
  {
    id: "kernel-invalid-opcode",
    test: (t) => /invalid opcode/i.test(t),
    title: "The CPU hit bytes that aren't an instruction",
    body: "Execution ran into data or garbage, typically from a `ret` to a wrong address, a bad function pointer, or a stack that was not laid out the way `ret` expects.",
  },
];

const match = (rules: Rule[], text: string) => rules.filter((r) => r.test(text)).map(({ id, title, body }) => ({ id, title, body }));

function findings(f: FailureInput): Finding[] {
  const out: Finding[] = [];
  const unchanged = f.code.trim() === f.starter.trim();

  if (f.compileFailed) {
    out.push(...match({ c: C_COMPILE, kernel: C_COMPILE, asm: ASM_COMPILE, go: GO_COMPILE, rust: RUST_COMPILE }[f.language], f.compileOutput));
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
  if (f.language === "go") out.push(...match(GO_RUNTIME, runtimeText));
  if (f.language === "kernel") out.push(...match(KERNEL_RUNTIME, runtimeText));
  const generic =
    f.language === "go" || f.language === "kernel"
      ? RUNTIME.filter((r) => r.id === "timeout")
      : f.language === "asm"
        ? RUNTIME.filter((r) => r.id !== "segfault" && r.id !== "ub-null" && r.id !== "abort")
        : RUNTIME;
  out.push(...match(generic, runtimeText));

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

  if (!out.length && (failed.length || f.error || f.runtimeFailed))
    out.push({
      id: "generic",
      title: "Something doesn't match yet",
      body: "Pick one failing test, work out by hand what your code does for that input, and compare it with what the test expected.",
    });

  // Warnings are only worth pointing out once there's a problem to explain.
  if (failed.length || f.error) out.push(...match(f.language === "c" ? C_WARN : [], f.compileOutput));
  return out.slice(0, 3);
}


export function explainFailure(f: FailureInput): Explanation[] {
  return findings(f).map((finding) => ({ ...finding, nextStep:
    f.compileFailed ? "Go to the first reported learner-code line, fix one error, then run Test again." :
    finding.id === "timeout" ? "Check that each loop makes progress and that Run has the input it needs." :
    finding.id === "unchanged" || finding.id === "placeholder" ? "Implement one function from the task, then test it before moving on." :
    finding.id === "whitespace" || finding.id === "case" || finding.id === "short" ? "Compare the first differing line in Expected and Your output, then check the printing statement." :
    f.runtimeFailed || /fault|abort|panic|bounds|unwrap|null|overflow/.test(finding.id) ? "Trace the smallest failing example, checking indices, pointers and values before the failing operation." :
    "Choose one failing test and trace your variables by hand; use a task example or a small input of your own if its input is hidden."
  }));
}
