// Sandbox escape regressions: hostile programs go through the real runner engine (runProgram), exactly as
// /api/run does. Each case must be contained; the script exits 1 if one is not.
// Needs gcc, rustc, go and bubblewrap (the same toolchain as verify:content). Run it on a disposable machine or CI,
// never on a shared host: the fork and memory bombs are real, just capped by rlimits (and cgroups when
// RUNNER_CGROUP_ROOT is set).
//   node --conditions=react-server --import tsx scripts/security/sandbox-escape.mts [language | name-filter]
import { execFileSync } from "node:child_process";
import { runProgram } from "../../src/lib/runner-engine";
import type { RunResult } from "../../src/lib/runner-types";

type Language = Parameters<typeof runProgram>[0];
type Check = (r: RunResult) => string | null;
type Case = { name: string; lang: Language; code: string; ok: Check };

const filter = process.argv[2];
const text = (r: RunResult) => `${r.compileOutput}\n${r.stdout}\n${r.stderr}`;
const leaksPasswd: Check = (r) => (/root:[x*]?:0:0/.test(text(r)) ? "contents of /etc/passwd reached the learner" : null);
const rejectedAtCompile: Check = (r) => (r.ok || r.phase !== "compile" ? `expected a compile-phase rejection, got ok=${r.ok} phase=${r.phase}` : null);
const printed = (m: string): Check => (r) => (r.stdout.includes(m) ? `"${m}" printed: the escape worked` : null);
const all = (...checks: Check[]): Check => (r) => checks.map((c) => c(r)).find(Boolean) ?? null;

const C = (body: string, includes = "") => `#define _GNU_SOURCE\n#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include <unistd.h>\n#include <fcntl.h>\n#include <errno.h>\n#include <dirent.h>\n#include <sys/socket.h>\n#include <netinet/in.h>\n#include <arpa/inet.h>\n${includes}\n${body}`;
const connectProbe = (ip: string, port: number) =>
  C(`int main(void){int s=socket(AF_INET,SOCK_STREAM,0);if(s<0){puts("NOSOCKET");return 0;}struct sockaddr_in a={.sin_family=AF_INET,.sin_port=htons(${port})};inet_pton(AF_INET,"${ip}",&a.sin_addr);if(connect(s,(void*)&a,sizeof a)==0){puts("CONNECTED");return 0;}puts("BLOCKED");return 0;}`);

const cases: Case[] = [
  {
    name: "fork bomb is bounded", lang: "c", ok: (r) => (r.timedOut || r.exitCode !== null ? null : "fork bomb neither finished nor timed out"),
    code: C(`int main(void){for(int i=0;i<100000;i++){pid_t p=fork();if(p==0){for(;;)pause();}if(p<0){puts("FORK_LIMIT");break;}}return 0;}`),
  },
  {
    name: "memory bomb hits the address-space limit", lang: "c",
    ok: (r) => { const all = [...r.stdout.matchAll(/ALLOC (\d+)/g)].map((m) => Number(m[1])); return Math.max(0, ...all) > 512 ? `allocated ${Math.max(...all)} MiB` : null; },
    code: C(`int main(void){size_t mb=0;for(;;){char*p=malloc(1<<20);if(!p)break;memset(p,1,1<<20);mb++;if(mb%16==0){printf("ALLOC %zu\\n",mb);fflush(stdout);}if(mb>4096)break;}printf("ALLOC %zu\\n",mb);return 0;}`),
  },
  {
    name: "huge zeroed static array is refused", lang: "c", ok: (r) => (r.ok && /DONE/.test(r.stdout) ? "a 2 GiB array was accepted" : null),
    code: C(`static char big[2UL<<30];\nint main(void){memset(big,1,sizeof big);puts("DONE");return 0;}`),
  },
  { name: "no network: loopback (MongoDB port)", lang: "c", ok: printed("CONNECTED"), code: connectProbe("127.0.0.1", 27017) },
  { name: "no network: public DNS server", lang: "c", ok: printed("CONNECTED"), code: connectProbe("1.1.1.1", 53) },
  {
    name: "no network: abstract unix socket of the host", lang: "c", ok: printed("CONNECTED"),
    code: C(`int main(void){int s=socket(AF_UNIX,SOCK_STREAM,0);struct sockaddr_un{unsigned short f;char p[108];}a={1,{0}};memcpy(a.p+1,"bm-runner",9);if(connect(s,(void*)&a,sizeof(short)+10)==0)puts("CONNECTED");else puts("BLOCKED");return 0;}`),
  },
  {
    name: "/proc shows no host processes and no secrets", lang: "c",
    ok: all(
      (r) => (/AUTH_SECRET|MONGODB|ANTHROPIC|SMTP_URL/.test(r.stdout) ? "host environment visible through /proc" : null),
      (r) => { const m = /PIDS (\d+)/.exec(r.stdout); return !m || Number(m[1]) > 8 ? `${m?.[1] ?? "?"} processes visible in /proc` : null; },
    ),
    code: C(`int main(void){int n=0;DIR*d=opendir("/proc");struct dirent*e;while(d&&(e=readdir(d)))if(e->d_name[0]>='0'&&e->d_name[0]<='9')n++;printf("PIDS %d\\n",n);const char*ps[]={"/proc/1/environ","/proc/self/environ","/proc/1/cmdline","/proc/1/root/etc/passwd","/proc/1/cwd"};for(int i=0;i<5;i++){char b[4096];int f=open(ps[i],O_RDONLY);if(f<0)continue;int k=read(f,b,sizeof b-1);if(k>0){b[k]=0;for(int j=0;j<k;j++)if(!b[j])b[j]=' ';printf("%s: %s\\n",ps[i],b);}}return 0;}`),
  },
  {
    name: "system directories (/usr, /lib, /sys, /proc/sys) are not writable", lang: "c", ok: printed("WRITABLE"),
    code: C(`int main(void){const char*ps[]={"/proc/sys/kernel/hostname","/usr/bin/x","/lib/x","/sys/x"};for(int i=0;i<5;i++){int f=open(ps[i],O_WRONLY|O_CREAT,0600);if(f>=0)printf("WRITABLE %s\\n",ps[i]);}return 0;}`),
  },
  { name: 'compile-time #include "/etc/passwd"', lang: "c", ok: leaksPasswd, code: C(`const char s[] = {\n#include "/etc/passwd"\n};\nint main(void){return 0;}`) },
  {
    name: "compile-time #include of runner and web environment files", lang: "c",
    ok: all(leaksPasswd, (r) => (/AUTH_SECRET|MONGODB_URI/.test(text(r)) ? "environment file contents leaked" : null)),
    code: C(`#include "/proc/self/environ"\n#include "/etc/baremetal/web.env"\n#include "/opt/baremetal-web/.env.local"\nint main(void){return 0;}`),
  },
  {
    name: "compile-time .incbin of /etc/passwd through asm()", lang: "c", ok: leaksPasswd,
    code: C(`__asm__(".section .rodata\\n.globl leak\\nleak: .incbin \\"/etc/passwd\\"\\n.byte 0\\n.text");\nextern const char leak[];\nint main(void){puts(leak);return 0;}`),
  },
  {
    name: "runtime read of /etc/passwd, /etc/shadow and /proc/1/root", lang: "c", ok: leaksPasswd,
    code: C(`int main(void){const char*ps[]={"/etc/passwd","/etc/shadow","/proc/1/root/etc/passwd"};for(int i=0;i<3;i++){FILE*f=fopen(ps[i],"r");char b[256];while(f&&fgets(b,sizeof b,f))fputs(b,stdout);}return 0;}`),
  },
  {
    name: "privileged syscalls fail (unshare, mount, ptrace, io_uring, bpf)", lang: "c",
    ok: (r) => (/SUCCEEDED/.test(r.stdout) ? `a denied syscall succeeded: ${r.stdout.trim()}` : null),
    code: C(`int main(void){if(unshare(CLONE_NEWUSER)==0)puts("SUCCEEDED unshare");if(mount("none","/tmp","tmpfs",0,0)==0)puts("SUCCEEDED mount");if(ptrace(PTRACE_ATTACH,1,0,0)==0)puts("SUCCEEDED ptrace");if(syscall(SYS_io_uring_setup,8,0)>=0)puts("SUCCEEDED io_uring");if(syscall(SYS_bpf,0,0,0)>=0)puts("SUCCEEDED bpf");if(access("/etc/shadow",R_OK)==0)puts("SUCCEEDED shadow");return 0;}`, "#include <sched.h>\n#include <sys/mount.h>\n#include <sys/ptrace.h>\n#include <sys/syscall.h>"),
  },
  {
    name: "rust: include_str! of a host file at compile time", lang: "rust", ok: leaksPasswd,
    code: `const S: &str = include_str!("/etc/passwd");\nfn main() { print!("{}", S); }`,
  },
  {
    name: "rust: a proc-macro cannot be built from a submission", lang: "rust", ok: rejectedAtCompile,
    code: `extern crate proc_macro;\nuse proc_macro::TokenStream;\n#[proc_macro]\npub fn evil(_: TokenStream) -> TokenStream { std::fs::read_to_string("/etc/passwd").unwrap().parse().unwrap() }\nfn main() {}`,
  },
  {
    name: "rust: spawning a shell reaches neither the network nor host files", lang: "rust", ok: all(leaksPasswd, printed("CONNECTED")),
    code: `use std::process::Command;\nfn main() { let o = Command::new("/usr/bin/sh").arg("-c").arg("cat /etc/passwd; exec 3<>/dev/tcp/127.0.0.1/27017 && echo CONNECTED").output(); if let Ok(o) = o { print!("{}", String::from_utf8_lossy(&o.stdout)); } }`,
  },
  {
    name: 'go: cgo is off (import "C" with a host include)', lang: "go",
    ok: all(leaksPasswd, (r) => (r.ok && r.stdout.includes("CGO") ? "cgo was active" : null)),
    code: `package main\n\n// #cgo LDFLAGS: -L/etc\n// #include "/etc/passwd"\n// const char *hello() { return "CGO"; }\nimport "C"\nimport "fmt"\n\nfunc main() { fmt.Println(C.GoString(C.hello())) }\n`,
  },
  {
    name: "go: //go:embed and os.ReadFile of host files", lang: "go", ok: leaksPasswd,
    code: `package main\n\nimport (\n\t_ "embed"\n\t"fmt"\n\t"os"\n)\n\n//go:embed /etc/passwd\nvar s string\n\nfunc main() { b, _ := os.ReadFile("/etc/passwd"); fmt.Println(s, string(b)) }\n`,
  },
  {
    name: "go: no network", lang: "go", ok: printed("CONNECTED"),
    code: `package main\n\nimport (\n\t"fmt"\n\t"net"\n\t"time"\n)\n\nfunc main() {\n\tfor _, a := range []string{"127.0.0.1:27017", "1.1.1.1:53"} {\n\t\tif c, err := net.DialTimeout("tcp", a, time.Second); err == nil {\n\t\t\tc.Close()\n\t\t\tfmt.Println("CONNECTED", a)\n\t\t}\n\t}\n}\n`,
  },
];

const procs = () => Number(execFileSync("sh", ["-c", "ps -u $(id -u) --no-headers | wc -l"]).toString().trim());
let failed = 0;
const before = procs();
for (const c of cases) {
  if (filter && !c.name.includes(filter) && c.lang !== filter) continue;
  const started = Date.now();
  let problem: string | null;
  try {
    const r = await runProgram(c.lang, c.code);
    problem = c.ok(r);
    if (problem) problem += ` | phase=${r.phase} ok=${r.ok} exit=${r.exitCode} stdout=${JSON.stringify(r.stdout.slice(0, 200))} compile=${JSON.stringify(r.compileOutput.slice(0, 200))}`;
  } catch (error) {
    problem = `engine threw: ${(error as Error).message}`;
  }
  if (problem) failed++;
  console.log(`${problem ? "FAIL" : "ok  "} [${c.lang}] ${c.name} (${Date.now() - started} ms)${problem ? `\n       ${problem}` : ""}`);
}

// Fork-bomb aftermath: nothing the sandbox spawned may linger, and the engine must still serve a normal run.
if (!filter) {
  await new Promise((r) => setTimeout(r, 1500));
  const after = procs();
  const sane = await runProgram("c", `#include <stdio.h>\nint main(void){puts("alive");return 0;}`);
  const bad = after > before + 5 ? `${after - before} processes left behind (fork bomb not reaped)` : sane.stdout.trim() !== "alive" ? "runner unusable after the bombs" : null;
  if (bad) failed++;
  console.log(`${bad ? "FAIL" : "ok  "} host recovers after the bombs${bad ? `: ${bad}` : ""}`);
}
console.log(failed ? `\n${failed} sandbox check(s) failed.` : "\nAll sandbox escape checks were contained.");
process.exit(failed ? 1 : 0);
