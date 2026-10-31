---
title: Reconstruct a mixing loop
summary: Recover a small checksum-style function from its assembly listing, including rotates, a multiply and the constants hidden in decimal.
minutes: 55
difficulty: hard
tags: security, reverse-engineering, assembly, hashing
id: sec-reverse-engineering/listing-hash
mode: harness
---
This listing is for a function that turns a buffer into a 32-bit number, `uint32_t mix(const unsigned char *p, size_t n)`. It is a made-up mixing function, not a real or standard hash. The arguments arrive in `rdi` (pointer) and `rsi` (length); the result is in `eax`. Constants in the listing are in decimal because that is how the disassembler printed them.

```
mix:
	test	rsi, rsi
	je	.L4
	mov	rax, rdi
	add	rdi, rsi
	mov	edx, 524114809
.L3:
	rol	edx, 5
	movzx	ecx, BYTE PTR [rax]
	xor	edx, ecx
	imul	edx, edx, 625341585
	add	rax, 1
	cmp	rax, rdi
	jne	.L3
.L2:
	mov	eax, edx
	shr	eax, 15
	xor	eax, edx
	ret
.L4:
	mov	edx, 524114809
	jmp	.L2
```

Read it in pieces. The compiler turned the loop into a pointer walk: `rax` is the current byte pointer and `rdi + rsi` was computed once as the end. The state lives in `edx`: it starts at the constant `524114809`, and on every byte it is rotated left by 5 (`rol`), xored with the byte (`movzx` zero-extends it, so it is unsigned), and multiplied by `625341585`. A 32-bit multiply wraps silently, which in C is exactly what `uint32_t` does. When the loop ends, `.L2` folds the high bits down: `eax = edx ^ (edx >> 15)`, a logical shift because it is `shr`. The empty-buffer case at `.L4` skips the loop and reaches the same final step with the starting constant.

Watch for three traps. The rotate is not a shift, so the high bits must come back in at the bottom (`(h << 5) | (h >> 27)`). The byte must be treated as unsigned. And the constants are the same numbers whatever base you print them in, so convert once and write them in hex if that helps you recognise them. As in every reconstruction, you get *a* function with the same behaviour; the original source and names are lost.
