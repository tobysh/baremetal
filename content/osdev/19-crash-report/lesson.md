---
title: Reading a Crash Report
summary: Turn raw exception numbers and error codes into text a human can act on: vector names, which vectors push an error code, page-fault flags, selector errors and a full report.
minutes: 35
difficulty: easy
tags: cpu, exceptions, debugging
id: osdev/crash-report
mode: harness
---
When a kernel crashes, the CPU hands your handler a few raw numbers: the **vector**, an **error code**, the **instruction pointer**, and for page faults the **CR2** register. A bare `vector 14, error 0x7` is useless at 3 a.m. The first debugging tool every kernel gets is a **crash report** that turns those numbers into a sentence. This lesson builds one, and teaches you how to read the numbers along the way.

## Vector names

The first 32 vectors belong to the CPU. The ones you will meet most:

| Vector | Name | Error code? |
|---|---|---|
| 0 | Divide Error | no |
| 3 | Breakpoint | no |
| 6 | Invalid Opcode | no |
| 8 | Double Fault | yes (always 0) |
| 10 | Invalid TSS | yes |
| 11 | Segment Not Present | yes |
| 12 | Stack-Segment Fault | yes |
| 13 | General Protection Fault | yes |
| 14 | Page Fault | yes |
| 17 | Alignment Check | yes |

Vectors 8, 10, 11, 12, 13, 14, 17, 21, 29 and 30 push an error code; all others do not (the boot stub pushes a zero in their place so the stack looks the same). The complete names are in the task below.

## The page-fault error code

For a page fault the error code is a set of flag bits, and **CR2** holds the address that was being accessed.

| Bit | Value 0 | Value 1 |
|---|---|---|
| 0 (P) | the page was **not present** | a **protection violation** on a present page |
| 1 (W) | the access was a read | the access was a write |
| 2 (U) | it happened in kernel mode | it happened in user mode |
| 3 (RSVD) | | a reserved bit was set in a page-table entry |
| 4 (I) | | it was an instruction fetch (with the NX bit) |

So `0x2` is "write to a non-present page from kernel mode": the classic null-pointer store. `0x7` is "write to a protected page from user mode": a user program writing to read-only memory.

## The selector error code

Faults like #GP (13), #NP (11), #SS (12) and #TS (10) push a **selector error code** when the problem was a descriptor:

| Bits | Meaning |
|---|---|
| 0 | the event was caused by something *external* to the program |
| 1..2 | which table: `0` GDT, `1` IDT, `2` LDT, `3` IDT |
| 3..15 | the **index** into that table (the selector shifted right by 3) |

An error code of 0 on a #GP is the common case "something other than a descriptor went wrong" (a privileged instruction, a non-canonical address); there is no selector to report.

## What the tests expect

Write short, fixed phrases so that the output is easy to test and easy to grep. The exact formats are in the task. Use the provided `put_str`, `put_dec` and `put_hex` helpers: each appends to a buffer without overrunning it and keeps it NUL-terminated, so your code only has to decide *what* to say.

## Debugging checklist

- **A fault reports the wrong page-fault kind:** check the bit tests; `(err & 2) != 0` is write, not `err == 2`.
- **Reserved vectors crash the lookup:** guard the table index with `vector < 22`.
- **Output is cut one character short:** the buffer's last byte is the NUL, so `cap - 1` characters fit.

## Your turn

Fill in the decoders and then the live handler, which prints a report for a real exception raised by the tests.
