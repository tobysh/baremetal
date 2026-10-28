// PoC for L1/L2: fake-pass programs per language, run through the runner directly.
// Usage: npx tsx scripts/security/integrity/poc-fakepass.mts [c|rust|go|asm]
import { getTracks } from "../../../src/lib/curriculum/index";
import { testExercise } from "../../../src/lib/runner";

const want = process.argv[2];
const pick = (lang: string) => {
  for (const t of getTracks()) for (const l of t.lessons) if (l.exercise.language === lang && l.exercise.mode !== "stdout" && l.exercise.harness) return l.exercise;
  return null;
};

const payload: Record<string, (starter: string) => string> = {
  c: (s) => `#include <unistd.h>\n#include <stdio.h>\n__attribute__((constructor(101))) static void bm_fake(void){ printf("\\n@@PASS fake\\n"); fflush(stdout); _exit(0); }\n${s}`,
  asm: (s) => s, // asm needs C constructor in .s: use .init_array with a raw syscall write+exit
  rust: (s) => `${s}\nextern "C" fn bm_fake() { use std::io::Write; let mut o = std::io::stdout(); let _ = o.write_all(b"test __bm_tests::fake ... ok\\n"); let _ = o.flush(); std::process::exit(0); }\n#[used] #[link_section = ".init_array"] static BM_FAKE: extern "C" fn() = bm_fake;\n`,
  go: (s) => `${s}\nfunc init() { println(); os_Stdout_write("--- PASS: Fake (0.00s)\\n"); os_Exit(0) }\n`,
};

const goExtra = `
import "os"
func os_Stdout_write(s string) { os.Stdout.WriteString(s) }
func os_Exit(c int) { os.Exit(c) }
`;

const asmFake = (s: string) => `.intel_syntax noprefix
.section .init_array,"aw"
.quad bm_fake_init
.text
bm_fake_init:
  mov eax, 1
  lea rsi, [rip+bm_msg]
  mov edi, 1
  mov edx, bm_msg_end - bm_msg
  syscall
  mov eax, 231
  xor edi, edi
  syscall
.section .rodata
bm_msg: .ascii "\\n@@PASS fake\\n"
bm_msg_end:
.text
${s.replace(/^\s*\.intel_syntax noprefix\s*$/m, "")}
`;

for (const lang of ["c", "rust", "go", "asm"]) {
  if (want && want !== lang) continue;
  const ex = pick(lang);
  if (!ex) { console.log(lang, "no harness exercise"); continue; }
  let code = lang === "asm" ? asmFake(ex.starter) : payload[lang](ex.starter);
  if (lang === "go") {
    // add import after package clause
    code = code.replace(/^(package main\s*)$/m, `$1\nimport "os"\n`).replace(/\nfunc init\(\).*\n/, `\nfunc init() { os.Stdout.WriteString("--- PASS: Fake (0.00s)\\n"); os.Exit(0) }\n`);
  }
  const real = await testExercise(ex, ex.starter);
  const r = await testExercise(ex, code);
  console.log(`[${lang}] ${ex.id}: starter allPassed=${real.allPassed}; FAKE compiled=${r.compiled} allPassed=${r.allPassed} results=${JSON.stringify(r.results)} err=${r.error} ${r.compiled ? "" : r.compileOutput.slice(0, 400)}`);
}
