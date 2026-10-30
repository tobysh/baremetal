#!/usr/bin/env bash
# L1/L2 PoC. Node/tsx was not installed on the audit host, so this replays the runner's exact
# source assembly (src/lib/runner.ts cTestSource/rustTestSource/compileGo/compileAsm), compile
# flags, bwrap sandbox and result parsers by hand. Usage: poc-fakepass.sh <c|rust|go|asm|leak>
set -u
C=$(cd "$(dirname "$0")/../../.." && pwd)/content
BW() { local w=$1; shift; bwrap --unshare-all --die-with-parent --new-session --ro-bind /usr /usr --symlink usr/lib /lib --symlink usr/lib64 /lib64 --symlink usr/bin /bin --ro-bind /etc/ld.so.cache /etc/ld.so.cache --proc /proc --dev /dev --tmpfs /tmp --bind "$w" /work --chdir /work --clearenv --setenv PATH /usr/bin --setenv HOME /work "$@"; }
PRE='#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#define CHECK(name, cond, ...) do { printf("\n@@RUN %s\n", name); fflush(stdout); if (cond) { printf("\n@@PASS %s\n", name); } else { printf("\n@@FAIL %s\t", name); printf(__VA_ARGS__); printf("\n"); } fflush(stdout); } while (0)'
INIT='#include <stdio.h>
__attribute__((constructor)) static void __bm_init(void) { setvbuf(stdout, NULL, _IOLBF, 0); }'
w=$(mktemp -d /tmp/bm-poc-XXXX); trap 'rm -rf "$w"' EXIT
case "$1" in
c)
  d=$C/c-memory/01-pointers-swap
  { printf '#include <unistd.h>\n#include <stdio.h>\n__attribute__((constructor(101))) static void bm_fake(void){ printf("\\n@@PASS fake\\n"); fflush(stdout); _exit(0); }\n'; cat $d/starter.c; printf '\n#undef main\n#line 1 "tests.c"\n%s\n' "$PRE"; cat $d/tests.c; } > $w/main.c
  echo "$INIT" > $w/bm_init.c
  BW $w gcc -std=c17 -g -O0 -fsanitize=undefined -Dmain=__bm_student_main main.c bm_init.c -o prog -lm 2>&1 | head
  BW $w ./prog; echo "exit=$?" ;;
rust)
  d=$C/rust-foundations/02-functions-and-types
  { cat $d/starter.rs; printf '\nextern "C" fn bm_fake() { use std::io::Write; let mut o = std::io::stdout(); let _ = o.write_all(b"test __bm_tests::fake ... ok\\n"); let _ = o.flush(); std::process::exit(0); }\n#[used] #[link_section = ".init_array"] static BM_FAKE: extern "C" fn() = bm_fake;\n\n#[cfg(test)]\nmod __bm_tests {\n#![allow(unused_imports)]\nuse super::*;\n'; cat $d/tests.rs; printf '\n}\n'; } > $w/main.rs
  BW $w rustc --edition 2021 -C opt-level=0 main.rs -o prog --test -A dead_code 2>&1 | head
  BW $w ./prog --test-threads=1 --color never; echo "exit=$?" ;;
go)
  d=$C/go-foundations/03-functions
  printf 'module bm\n\ngo 1.24\n' > $w/go.mod
  { cat $d/starter.go; printf '\nfunc init() { os.Stdout.WriteString("--- PASS: Fake (0.00s)\\n"); os.Exit(0) }\n'; } | sed '0,/^package main/s//package main\nimport "os"/' > $w/main.go
  cp $d/tests.go $w/bm_test.go
  BW $w --setenv GOCACHE /tmp/gocache --setenv GOPATH /tmp/gopath --setenv GOFLAGS "-mod=mod -buildvcs=false" --setenv GOPROXY off --setenv GOTOOLCHAIN local --setenv CGO_ENABLED 0 --setenv GOTELEMETRY off go test -c -vet=off -o prog 2>&1 | head
  BW $w ./prog -test.v -test.count=1; echo "exit=$?" ;;
asm)
  d=$C/asm-first-steps/01-hello-registers
  { cat <<'ASM'
.intel_syntax noprefix
.section .init_array,"aw"
.quad bm_fake_init
.text
bm_fake_init:
  mov eax, 1
  lea rsi, [rip+bm_msg]
  mov edi, 1
  mov edx, 14
  syscall
  mov eax, 231
  xor edi, edi
  syscall
.section .rodata
bm_msg: .ascii "\n@@PASS fake\n\n"
.text
ASM
  grep -v '^.intel_syntax' $d/starter.s; } > $w/starter.s
  echo "$INIT" > $w/bm_init.c
  printf '%s\n#line 1 "tests.c"\n' "$PRE" > $w/tests.c; cat $d/tests.c >> $w/tests.c
  BW $w gcc -std=c17 -g -O0 -no-pie -Wa,--noexecstack starter.s bm_init.c tests.c -o prog 2>&1 | head
  BW $w ./prog; echo "exit=$?" ;;
leak)
  # L2: learner code reads the harness source at run time
  d=$C/c-memory/01-pointers-swap
  { printf '#include <unistd.h>\n#include <stdio.h>\n__attribute__((constructor(101))) static void bm_leak(void){ FILE*f=fopen("main.c","r"); char b[8192]; size_t n=fread(b,1,sizeof b-1,f); b[n]=0; fputs(b,stdout); fflush(stdout); _exit(0);}\n'; cat $d/starter.c; printf '\n#undef main\n#line 1 "tests.c"\n%s\n' "$PRE"; cat $d/tests.c; } > $w/main.c
  echo "$INIT" > $w/bm_init.c
  BW $w gcc -std=c17 -O0 -Dmain=__bm_student_main main.c bm_init.c -o prog 2>&1 | head
  BW $w ./prog | tail -25; BW $w ls /work ;;
esac
