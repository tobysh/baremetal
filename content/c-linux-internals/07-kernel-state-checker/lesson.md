---
title: Capstone, a Kernel-State Checker
summary: Combine descriptors, credentials, signals and processes in one simulated kernel, then write open, exit and a checker that names every broken invariant.
minutes: 60
difficulty: hard
tags: capstone, kernel-model, invariants
id: c-linux-internals/kernel-state-checker
mode: harness
---
The earlier lessons each modelled one subsystem. Real kernels are correct only if the pieces agree with each other. This capstone puts them in one `kstate` and asks you to write the code that enforces and checks that agreement.

```c
typedef struct {
    sysfiles files;     // open file descriptions
    fdtable  fdt[NPROC];
    ptable   pt;
    sproc    sig[NPROC];
    cred     cr[NPROC];
} kstate;
```

All the earlier code (descriptor functions, `may_access`, signal functions, process table functions) is given. You write three functions that combine them.

## The invariants

`ks_check` returns the OR of these bits (the starter defines the names):

| Bit | Violated when |
|-----|---------------|
| `V_DANGLING` | a **running** process has a slot that is neither -1 nor a valid index, or that names a description with `refs <= 0` |
| `V_REFCOUNT` | a description has `refs < 0`, or `refs > 0` and different from the number of slots (in any process, any state) naming it |
| `V_DEAD_FDS` | a zombie or free process still has a slot other than -1 |
| `V_PARENT` | an existing pid >= 2 has a parent that is not in `1..NPROC-1` or not running |
| `V_CYCLE` | walking parent links from an existing pid >= 2 neither reaches pid 1 nor stops at a missing process (it loops) |
| `V_SIG_DEAD` | a zombie or free process has pending signals |
| `V_SIG_UNBLOCKABLE` | an existing process has `SIGKILL`, `SIGSTOP` or bit 0 in its blocked mask |
| `V_INIT` | pid 1 is not running, or its `ppid` is not 0 |

The bits are independent: a broken parent link reports `V_PARENT` only, not `V_CYCLE`, unless it also loops.

## The two operations

- `ks_open` combines permissions and descriptors: refuse unless the process is running and `may_access` passes for **that process's credentials**, then open.
- `ks_exit` combines processes, descriptors and signals. Every descriptor is closed (which can free descriptions), pending signals are discarded, the process becomes a zombie, children are adopted by init, and the parent gets `SIGCHLD`, which the signal rules may immediately discard if the parent ignores it. A refused exit changes nothing.

## What the tests do and do not show

The hidden tests build states directly, inject one, two or three defects at a time, and compare your result with a separately written checker. They also run random exits and require your state to match a reference and to pass your own `ks_check`. They cannot grade whether you understand *why* each invariant matters; for each bit, write one sentence about the real kernel bug it would indicate. The model is the one stated in lessons 1 to 6, with fixed small tables.

## Your turn

Implement `ks_open`, `ks_exit` and `ks_check`. Read the public example first.
