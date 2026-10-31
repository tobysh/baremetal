---
title: Capstone: Link and Run
summary: Write the executable emitter and link map, then link two objects into an inspectable image and run it on a provided tiny VM.
minutes: 60
difficulty: hard
tags: linker, capstone, vm
id: go-executable-formats/link-and-run
mode: harness
---
Everything comes together. You have parsed objects, resolved names, laid out sections and applied relocations. Now you emit the executable, produce a link map, and run the result.

## Outcome

`LinkAndRun` takes two or more object files and an entry name and returns the executable bytes, a link map, an `Describe` report of the parsed executable, and the output of running the program on the TVM, a tiny register machine given in the starter. A failure at any stage is reported by cause and nothing runs unless the link succeeded.

## The TVM

Four 32-bit registers, an eleven-instruction set (`LI`, `LOAD`, `STORE`, `ADD`, `SUB`, `CALL`, `RET`, `JNZ`, `JMP`, `OUT`, `HALT`; the table is in the starter), memory made of the executable's segments, and permissions enforced on every access: instructions are fetched only from executable segments, stores go only to writable ones. A bad access is `ErrFault`; exceeding the step budget is `ErrSteps`. The machine is provided and trusted. It is a model of the course's format: a program that runs here proves your relocations are right for *this* machine, not that they would be right for any real CPU.

## Inspectable artifacts

Three artifacts let you check a link without running it: the **link map** (every global symbol with its final address), the **description** of the executable (segments, permissions, entry) and the bytes themselves. The tests rely on them: they compute the map and the whole file with an independent model and require your file to be byte-identical, so the image layout (header, segment table, text, data, no padding) is part of the specification.

## Properties to expect

Linking `[main, lib]` and `[lib, main]` must give different addresses but the same program output, since only the layout changed. Entry points, data pointers and calls into the other object all depend on relocations being right. Linking must not alter its inputs, and a program that faults still reports the output printed before the fault.
