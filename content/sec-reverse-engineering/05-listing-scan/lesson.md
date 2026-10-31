---
title: Rebuild a C function from a listing
summary: Read an x86-64 assembly listing in Intel syntax, map registers and jumps back to variables and conditions, and write equivalent C.
minutes: 50
difficulty: medium
tags: security, reverse-engineering, assembly, decompilation
id: sec-reverse-engineering/listing-scan
mode: harness
---
Below is the compiled form of a small function that is not shown in source. It is plain text, supplied with this lesson; nothing runs it. The calling convention is System V x86-64: the first argument is in `rdi`, the second in `rsi`, and the result is returned in `eax`. The function is called as `int scan(const int *a, int n)`, so `rdi` is the array and `esi` (the low half of `rsi`) is `n`.

```
scan:
	test	esi, esi
	jle	.L8
	movsxd	rsi, esi
	mov	eax, 0
	mov	ecx, 0
	mov	r8d, -1
	jmp	.L7
.L9:
	mov	r8d, eax
	mov	ecx, edx
.L4:
	add	rax, 1
	cmp	rax, rsi
	je	.L1
.L7:
	mov	edx, DWORD PTR [rdi+rax*4]
	test	edx, edx
	jle	.L4
	cmp	edx, ecx
	jl	.L9
	test	r8d, r8d
	js	.L9
	jmp	.L4
.L8:
	mov	r8d, -1
.L1:
	mov	eax, r8d
	ret
```

A method that works on any listing. First give every register a role: `rax` counts up by one and is compared with `rsi` (the length), so it is the loop index `i`; `[rdi+rax*4]` is `a[i]`, loaded into `edx`. Then read the compare-and-jump pairs as conditions. `test edx, edx` followed by `jle` jumps when the value is less than or equal to zero, so such elements are skipped. `cmp edx, ecx` then `jl .L9` jumps to the update block when `edx < ecx`. `test r8d, r8d` then `js` jumps when `r8d` is negative. The update block `.L9` copies `eax` into `r8d` and `edx` into `ecx`, so `r8d` remembers a position and `ecx` the value found there. Finally `.L1` returns `r8d`, which started at -1.

The compiler reordered the tests, so your C need not look the same, only behave the same on every input. Note what the listing does *not* tell you: it does not name anything. Your variable names are your interpretation. The value of the analysis is in how the tests were chosen: several arrays, including ties and an empty one, probe whether you copied the rule or only a sample of it.
