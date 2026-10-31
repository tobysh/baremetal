---
title: Step Machines
summary: Model a thread as a program counter and registers, and write the two functions that decide whether it can move and execute one step.
minutes: 30
difficulty: easy
tags: model-checking, scheduler, state-machine
id: c-model-checking/step-machines
mode: harness
---
Real threads are hard to test because the operating system picks the interleaving and you cannot choose it. A **model checker** replaces the threads with a model you drive yourself. Nothing in this track uses `pthread`.

A thread becomes a **step machine**: a program counter `pc`, a few registers, and a list of tiny instructions. A *scheduler* decides which thread takes the next step. A **schedule** is the list of thread ids in the order they ran, for example `0 1 0 0 1 1`.

## The instruction set

Each thread is an array of `insn` ending in `OP_END`. One instruction is one atomic step; `r[x]` is a register of the running thread and `m[x]` is shared memory. The full table is in the starter's comments: `SETI`, `MOV`, `ADDI`, `LOAD`, `STORE`, `STOREX`, `SLTI`, `JZ`, `JNZ`, `JMP`, `LOCK`, `UNLOCK`, `CAS` and `END`. `LOCK a` can only run while lock `a` is free; that is what blocks threads.

## The classic race

Two threads each run `LOAD r0,m[0]; ADDI r0,1; STORE m[0],r0`. The schedule `0 0 0 1 1 1` ends with `m[0] = 2`. The schedule `0 1 0 0 1 1` lets thread 1 load before thread 0 stores, so one increment is lost and `m[0] = 1`. The starter's `main` shows this step by step.

## What you write

`thread_enabled` and `step` are the heart of the model. Everything later in the track calls them.

**Limitation:** this is a model. Each instruction is atomic and steps run in the order chosen, so the model has no CPU reordering and no torn reads. Later lessons say what that means for conclusions. A graphical stepper that replays a schedule is future work; here a schedule is a list of numbers.
