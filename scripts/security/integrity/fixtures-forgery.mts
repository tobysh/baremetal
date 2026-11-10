// Regression fixtures for Phase 1 of docs/security.md (R1, R3).
// Each fixture is a learner submission that must NOT pass; the script exits 1 if one does.
// Usage: npx tsx scripts/security/integrity/fixtures-forgery.mts [c|rust|go|asm|kernel]
import { findExercise, getTracks } from "../../../src/lib/curriculum/index";
import { referenceTranscript, testExercise } from "../../../src/lib/runner";
import type { Exercise } from "../../../src/lib/curriculum/types";

const want = process.argv[2];
const pick = (lang: string): Exercise | null => {
  for (const t of getTracks()) for (const l of t.lessons) if (l.exercise.language === lang && l.exercise.mode === "harness" && l.exercise.harness) return l.exercise;
  return null;
};

// `expect`: the rejection must come from this specific check, not from the starter merely failing.
type Fixture = { lang: string; name: string; build: (ex: Exercise) => string; expect?: RegExp };
const REACH = /could reach the hidden tests/;
const TAMPER = /belongs to the test harness/;

// One real test's worth of output, then a clean exit: before R1 this passed a lesson.
const fixtures: Fixture[] = [
  {
    lang: "c",
    name: "runtime-built PASS line then _exit(0) (constructor hidden from the scan)",
    build: (ex) => `#include <unistd.h>\n#include <stdio.h>\n#include <string.h>\n#include <fcntl.h>\n#define C __attribute__((__constructor__(101)))\nC static void f(void){ char p[32]; snprintf(p,32,"/pro%c/self/exe",99); int fd=open(p,O_RDONLY); static char m[1<<20]; int n=read(fd,m,sizeof m); char t[8]; snprintf(t,8,"%c%cRUN ",64,64); for(int i=0;i<n-30;i++) if(!memcmp(m+i,t,6)){ char o[96]; int k=snprintf(o,96,"\\n%c%cPASS %.24s forged\\n",64,64,m+i+6); write(1,o,k); _exit(0);} }\n${ex.starter}`,
  },
  {
    lang: "c",
    name: "reads BM_NONCE from /proc/self/environ at run time and forges a PASS (R2)",
    build: (ex) => `#include <unistd.h>\n#include <stdio.h>\n#include <string.h>\n#include <fcntl.h>\n#define C __attribute__((__constructor__(101)))\nC static void f(void){ char p[32]; snprintf(p,32,"/pro%c/self/environ",99); int fd=open(p,O_RDONLY); static char m[8192]; int n=read(fd,m,sizeof m-1); m[n>0?n:0]=0; for(int i=0;i<n;i++) if(!strncmp(m+i,"BM_NONCE=",9) && m[i+9]){ char o[96]; int k=snprintf(o,96,"\\n%c%cPASS %.24s forged\\n",64,64,m+i+9); write(1,o,k); _exit(0);} }\n${ex.starter}`,
  },
  {
    lang: "c",
    name: "quoted #include of the hidden tests (R3)",
    build: (ex) => `#include "tests.c"\n${ex.starter}`,
  },
  {
    lang: "rust",
    name: "include_str! of the hidden tests (R3)",
    build: (ex) => `${ex.starter}\nconst _LEAK: &str = include_str!("main.rs");\n`,
  },
  {
    lang: "go",
    name: "go:embed of the hidden tests (R3)",
    build: (ex) => `${ex.starter}\n//go:embed bm_test.go\nvar leak string\n`,
  },
  {
    lang: "asm",
    name: ".incbin of the hidden tests (R3)",
    build: (ex) => `${ex.starter}\n.section .rodata\nleak: .incbin "tests.c"\n`,
  },
  {
    lang: "c",
    name: "split-string .incbin of the combined source (R3 spelling variant)",
    build: (ex) => `${ex.starter}\n__asm__(".section .rodata\\nleak: .inc" "bin \\"main.c\\"\\n.text");\n`,
    expect: REACH,
  },
  {
    lang: "kernel",
    name: "split-string .incbin of the combined kernel source (nonce leak)",
    build: (ex) => `${ex.starter}\n__asm__(".section .rodata\\nleak: .inc" "bin \\"kernel.c\\"\\n.text");\n`,
    expect: REACH,
  },
  {
    lang: "c",
    name: "#include through a macro that names the combined source",
    build: (ex) => `${ex.starter}\n#define F "main.c"\n#include F\n`,
    expect: REACH,
  },
  {
    lang: "kernel",
    name: "#include through a macro that names the combined kernel source",
    build: (ex) => `${ex.starter}\n#define F "kernel.c"\n#include F\n`,
    expect: REACH,
  },
  // The guard compiles the learner's file alone; these emit the directive only in the real build, so
  // only the scan of the real combined assembly (compileToAssembly) or the text ban can stop them.
  {
    lang: "c",
    name: "__has_include(bm_init.c) gates a split-string .incbin of main.c",
    build: (ex) => `#if __has_include("bm_init.c")\n__asm__(".section .rodata\\nleak: .inc" "bin \\"main.c\\"\\n.text");\n#endif\n${ex.starter}`,
    expect: TAMPER,
  },
  {
    lang: "kernel",
    name: "__has_include_next(runtime.c) gates a split-string .incbin of kernel.c",
    build: (ex) => `#if __has_include_next("runtime.c")\n__asm__(".section .rodata\\nleak: .inc" "bin \\"kernel.c\\"\\n.text");\n#endif\n${ex.starter}`,
    expect: TAMPER,
  },
  {
    lang: "c",
    name: "real-build-only macro (__SSP_STRONG__) gates a split-string .incbin of main.c",
    build: (ex) => `#ifdef __SSP_STRONG__\n__asm__(".section .rodata\\nleak: .inc" "bin \\"main.c\\"\\n.text");\n#endif\n${ex.starter}`,
    expect: REACH,
  },
  {
    lang: "c",
    name: "token-pasted !__has_include(bm_base.c) gates a split-string .incbin of main.c",
    build: (ex) => `#define P(a, b) a##b\n#if !P(__has_, include)("bm_base.c")\n__asm__(".section .rodata\\nleak: .inc" "bin \\"main.c\\"\\n.text");\n#endif\n${ex.starter}`,
    expect: REACH,
  },
  {
    lang: "kernel",
    name: "token-pasted !__has_include(bm_base.c) gates a split-string .incbin of kernel.c",
    build: (ex) => `#define P(a, b) a##b\n#if !P(__has_, include)("bm_base.c")\n__asm__(".section .rodata\\nleak: .inc" "bin \\"kernel.c\\"\\n.text");\n#endif\n${ex.starter}`,
    expect: REACH,
  },
  {
    lang: "c",
    name: "absolute <...> include of a file outside /usr, gated to the real build (dependency check)",
    build: (ex) => `#define P(a, b) a##b\n#if !P(__has_, include)("bm_base.c")\n#include </dev/null>\n#endif\n${ex.starter}`,
    expect: REACH,
  },
  {
    lang: "kernel",
    name: ".incbin hidden behind a C comment in the assembly, gated to the real build",
    build: (ex) => `#define P(a, b) a##b\n#if !P(__has_, include)("bm_base.c")\n__asm__(".section .rodata\\nleak: /* x */ .inc" "bin \\"kernel.c\\"\\n.text");\n#endif\n${ex.starter}`,
    expect: REACH,
  },
  {
    lang: "c",
    name: "#define if(c) if(1) makes every CHECK pass",
    build: (ex) => `${ex.starter}\n#define if(c) if (1)\n`,
  },
  {
    lang: "kernel",
    name: "#define if(c) if(1) makes every CHECK pass",
    build: (ex) => `${ex.starter}\n#define if(c) if (1)\n`,
  },
  {
    lang: "kernel",
    name: "#define else makes the failing branch vanish",
    build: (ex) => `${ex.starter}\n#define else if (0)\n`,
  },
  {
    lang: "rust",
    name: "include_str ! with a space before the bang (R3 spelling variant)",
    build: (ex) => `${ex.starter}\nconst _LEAK: &str = include_str !("main.rs");\n`,
  },
  {
    lang: "rust",
    name: "macro forwards the include_str name (R3 indirection)",
    build: (ex) => `${ex.starter}\nmacro_rules! m { ($n:ident) => { $n!("main.rs") } }\nconst _LEAK: &str = m!(include_str);\n`,
  },
  {
    lang: "rust",
    name: "#[path] module pointing at the harness file (R3)",
    build: (ex) => `${ex.starter}\n#[path = "main.rs"]\nmod leak;\n`,
  },
  {
    lang: "asm",
    name: "upper-case .INCBIN of the hidden tests (R3 case variant)",
    build: (ex) => `${ex.starter}\n.section .rodata\nleak: .INCBIN "tests.c"\n`,
  },
  {
    lang: "asm",
    name: ".macro builds the directive name at assembly time (R3 indirection)",
    build: (ex) => `${ex.starter}\n.macro d x\n.\\x "tests.c"\n.endm\n.section .rodata\nd incbin\n`,
  },
  {
    lang: "c",
    name: "C23 #embed of the hidden tests (R3)",
    build: (ex) => `${ex.starter}\nstatic const char leak[] = {\n#embed "main.c"\n, 0};\n`,
  },
  {
    lang: "go",
    name: "init() prints a PASS line then exits",
    build: (ex) => ex.starter.replace(/^package main\s*$/m, 'package main\nimport "os"\nfunc init() { os.Stdout.WriteString("--- PASS: Fake (0.00s)\\n"); os.Exit(0) }'),
  },
];

let leaked = 0;
for (const f of fixtures) {
  if (want && want !== f.lang) continue;
  const ex = pick(f.lang);
  if (!ex) { console.log(`skip  [${f.lang}] no harness exercise`); continue; }
  const r = await testExercise(ex, f.build(ex));
  const bad = r.allPassed || (f.expect ? !f.expect.test(r.error ?? "") : false);
  if (bad) leaked++;
  console.log(`${bad ? "FAIL" : "ok  "} [${f.lang}] ${f.name} -> allPassed=${r.allPassed} compiled=${r.compiled} error=${r.error ?? "-"} ${r.compiled ? "" : r.compileOutput.slice(0, 700)}`);
}
console.log(leaked ? `\n${leaked} forged submission(s) passed.` : "\nAll forged submissions were rejected.");

// Compiler output must never quote hidden harness lines (R3). Redefining CHECK makes gcc
// warn about the prelude, which contains this run's nonce.
if (!want || want === "c") {
  const ex = pick("c");
  if (ex) {
    const r = await testExercise(ex, `#define CHECK(...) 1\n${ex.starter}`);
    const bad = /@@(RUN|PASS|FAIL)|tests\.c:\d+/.test(r.compileOutput);
    if (bad) leaked++;
    console.log(`${bad ? "FAIL" : "ok  "} [c] CHECK redefinition does not echo the prelude in compile output`);
  }
}
// Out-of-process verdict (driver.c lessons, src/lib/verdict.ts). The verdict is taken by comparing
// transcripts outside the learner's process, so each of these must fail even though the learner
// controls its whole process, including fd 3.
if (!want || want === "c") {
  const swapEx = findExercise("c-memory/pointers-swap")?.exercise;
  const arraysEx = findExercise("c-data/arrays")?.exercise;
  if (!swapEx?.driver || !arraysEx?.driver) { console.log("FAIL [c] driver lessons c-memory/pointers-swap and c-data/arrays are missing their driver.c"); leaked++; }
  else {
    const cString = (text: string) => JSON.stringify(text).replace(/\\u00([0-9a-f]{2})/g, "\\x$1");
    // A complete, valid transcript recorded under another seed, replayed on stdout and fd 3.
    const replay = await referenceTranscript(swapEx, Buffer.alloc(16, 0x5a));
    if (!replay.ok) { console.log(`FAIL [c] reference transcript unavailable: ${replay.reason}`); leaked++; }
    const ctor = "#include <unistd.h>\n#include <fcntl.h>\n#include <string.h>\n#include <stdio.h>\n#define C __attribute__((__constructor__(101)))\n";
    const swapFixed = `#include <stdio.h>
void swap(int *a, int *b) {
    static const int known[][4] = {{1, 2, 2, 1}, {-7, 100, 100, -7}, {0, 55, 55, 0}, {9, 9, 9, 9}, {-2147483647 - 1, 2147483647, 2147483647, -2147483647 - 1}, {3, 4, 4, 3}, {4, 3, 3, 4}, {10, 30, 30, 10}};
    for (int i = 0; i < 8; i++) if (*a == known[i][0] && *b == known[i][1]) { *a = known[i][2]; *b = known[i][3]; return; }
}
`;
    type DriverFixture = { name: string; ex: typeof swapEx; code: string; check?: (r: Awaited<ReturnType<typeof testExercise>>) => string | null };
    const driverFixtures: DriverFixture[] = [
      ...(replay.ok ? [{
        name: "replays a complete transcript from another seed on fd 3 and stdout, then _exit(0)",
        ex: swapEx,
        code: `${ctor}C static void f(void){ const char *t = ${cString(replay.text)}; write(3, t, strlen(t)); write(1, t, strlen(t)); _exit(0); }\n${swapEx.starter}`,
      }] : []),
      {
        name: "hardcodes the answers to every fixed edge case",
        ex: swapEx,
        code: swapFixed,
        check: (r) => r.results.some((t) => t.name.includes("random") && t.passed) ? "a seeded case passed with hardcoded answers" : r.results.some((t) => t.name.includes("random") && /Expected/.test(t.message ?? "")) ? "a seeded case revealed its expected values" : null,
      },
      {
        name: "writes an extra (correct-looking) record to fd 3 inside the call",
        ex: swapEx,
        code: `#include <unistd.h>\nvoid swap(int *a, int *b) { int t = *a; *a = *b; *b = t; write(3, "O i 78 1\\n", 9); }\n`,
      },
      {
        name: "writes the end record to fd 3 and exits after the first call",
        ex: swapEx,
        code: `#include <unistd.h>\nvoid swap(int *a, int *b) { int t = *a; *a = *b; *b = t; write(3, "E\\n", 2); _exit(0); }\n`,
      },
      {
        name: "assembles an .incbin of driver.c at compile time",
        ex: arraysEx,
        code: `${arraysEx.solution}\n__asm__(".section .rodata\\nleak: .inc" "bin \\"driver.c\\"\\n.text");\n`,
        check: (r) => r.compiled ? "the learner's object compiled although driver.c should not exist in its build directory" : null,
      },
      {
        name: "opens the driver, the reference and the seed pipe at run time (the correct solution still passes, nothing leaks)",
        ex: arraysEx,
        code: `${ctor}C static void f(void){ const char *p[] = {"driver.c", "driver.o", "/input/driver.c", "solution.c", "main.c", "/work/main.o"}; for (int i = 0; i < 6; i++) if (open(p[i], O_RDONLY) >= 0) printf("LEAKED %s\\n", p[i]); char b[16]; if (read(4, b, 16) > 0) printf("LEAKED seed\\n"); }\n${arraysEx.solution}`,
        check: (r) => /LEAKED/.test(r.output) ? `read something: ${r.output}` : !r.allPassed ? `the correct solution failed: ${r.error}` : null,
      },
    ];
    for (const f of driverFixtures) {
      const r = await testExercise(f.ex, f.code);
      const problem = f.check ? f.check(r) : r.allPassed ? "passed" : null;
      const bad = !!problem || (!f.check && r.allPassed);
      if (bad) leaked++;
      console.log(`${bad ? "FAIL" : "ok  "} [c driver] ${f.name} -> allPassed=${r.allPassed} compiled=${r.compiled} error=${r.error ?? "-"}${problem ? ` problem=${problem}` : ""}`);
    }
  }
}
console.log(leaked ? `\n${leaked} check(s) failed in total.` : "All checks passed.");
process.exit(leaked ? 1 : 0);
