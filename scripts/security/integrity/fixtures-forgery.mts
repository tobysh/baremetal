// Regression fixtures for Phase 1 of docs/security-remediation-plan.md (R1, R3).
// Each fixture is a learner submission that must NOT pass; the script exits 1 if one does.
// Usage: npx tsx scripts/security/integrity/fixtures-forgery.mts [c|rust|go|asm]
import { getTracks } from "../../../src/lib/curriculum/index";
import { testExercise } from "../../../src/lib/runner";
import type { Exercise } from "../../../src/lib/curriculum/types";

const want = process.argv[2];
const pick = (lang: string): Exercise | null => {
  for (const t of getTracks()) for (const l of t.lessons) if (l.exercise.language === lang && l.exercise.mode === "harness" && l.exercise.harness) return l.exercise;
  return null;
};

type Fixture = { lang: string; name: string; build: (ex: Exercise) => string };

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
  const bad = r.allPassed;
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
console.log(leaked ? `\n${leaked} check(s) failed in total.` : "All checks passed.");
process.exit(leaked ? 1 : 0);
