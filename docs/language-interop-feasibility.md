# Language interoperability: can the runner link C into Rust or Go?

Status: investigated 30 October 2026. Answer: **no**, not without runner changes. No runner code was changed.

## What the runner does today (`src/lib/runner-engine.ts`)

- **Rust** (`compile()`, last branch): the learner's source is written to a single `main.rs` and built with
  `rustc --edition 2021 -C opt-level=0 -C linker=/usr/bin/gcc main.rs -o prog` (plus `--test` for harness lessons).
  gcc is used only as the *linker driver*. There is no step that compiles a C file, no `-l`/`-L` flags, no
  `build.rs`, no Cargo and no `cc` crate. Harness tests are appended to the same file.
- **Go** (`compileGo()`): a throwaway module with `main.go` (and the hidden `bm_test.go`), `go build` / `go test -c`
  with `CGO_ENABLED=0`, `GOPROXY=off`. With cgo off, `import "C"` files are excluded from the build, so a
  cgo program fails to compile ("build constraints exclude all Go files" or "C source files not allowed when
  not using cgo"). Go also has no way to take a second language's source file in this module layout.
- **C/asm** each have their own path; only asm links a hidden C harness, and that is the other way round (learner
  assembly called from hidden C), not a learner-visible multi-language build.

What *is* possible today: a Rust program can declare `extern "C"` for symbols already in libc (`strlen`, `qsort`, `abs`)
because std links libc, so one could call real C library code. That does not teach writing or linking C, and it
depends on the host libc inside the bubblewrap view, so it is not a basis for a course on its own.

## What is missing

1. A second source file per lesson (`helper.c`), carried through the API, the editor (tabs) and the hidden-test
   protocol. Today an exercise is one learner file plus one hidden harness.
2. A C compile step before the Rust/Go build: `gcc -c helper.c`, then `ar` into a static archive (or an object file).
3. Rust: `-C link-arg=helper.o` or `-l static=helper -L .` on the rustc line (or `#[link]`), plus bindings declared by
   the learner. Straightforward once 1 and 2 exist; the sandbox already has gcc.
4. Go: `CGO_ENABLED=1` with `CC=/usr/bin/gcc`, a `.c`/`.h` file in the module (or `import "C"` preamble code), and
   cgo's helper tools (`cgo`, `gcc`, `ld`) reachable in the sandbox. The cold standard-library build is slower with
   cgo (runtime/cgo, net), so the 60 s compile budget and the warmed `GO_CACHE_SEED` would need a cgo variant
   (cache keys differ when cgo is on). `go test -c` with cgo also links with gcc, which needs a larger address-space limit.
5. Security review: compiling attacker-written C inside the Go/Rust compile sandbox widens the surface
   (`#include "/proc/..."`, `.incbin`, `#embed`). The existing tamper patterns in `runner-engine.ts` cover only the
   single-file cases and would need a C-source variant (they already forbid `#include "` in all languages).
6. Verifier support: `scripts/verify-curriculum.mts` assumes one `starter.<ext>`/`solution.<ext>` pair.

## Recommended runner change, if wanted later

Add an optional per-lesson hidden or visible `support.c` (hidden is simplest and avoids the editor work): the
runner compiles it with the existing C flags to `support.o`, and passes it to rustc via `-C link-arg=support.o`.
That gives a Rust-calls-C course with minimal new surface (no learner-written C). Go needs the cgo work in item 4 and
is the expensive half; defer it.

## What was built instead

`content/rust-ffi-boundaries/`: ABI-boundary concepts in pure Rust, no C toolchain: `#[repr(C)]` layout, `extern "C"`
functions and function pointers, callbacks with context pointers, raw-pointer ownership, `Box::into_raw`/`from_raw`
handles, error codes and panic containment, snprintf-style buffers. Everything is exercised from Rust on both sides of
the "boundary". Honest limit: nothing here proves compatibility with a real C compiler's ABI choices; leak and
use-after-free detection beyond the drop counters used in lessons 5 and 7 would need Miri or Valgrind, which the
runner does not provide.
