---
title: Resolving Symbols
summary: Build the global symbol table across objects: match references to definitions, report duplicates and unresolved names, keep locals private.
minutes: 40
difficulty: medium
tags: linker, symbols, name-resolution
id: go-executable-formats/resolving-symbols
mode: harness
---
When the compiler built `main.o` it did not know where `double` lives; it left an *undefined symbol* named `double`. The linker's first job is **symbol resolution**: find which object defines each name.

## Outcome

`Resolve` maps every reference to exactly one definition, or fails with an error that names the symbol.

## Rules

- **Global** symbols are visible to all objects; **local** symbols are private to the object that defines them. Two objects can both have a local `helper`. A local never satisfies another object's reference, and a local with the same name as someone's global is not a clash.
- Two global definitions of one name are a **duplicate symbol** error. Which definition wins is not decided by object order; the linker refuses.
- A reference with no definition anywhere is an **undefined symbol** error.

Real linkers add weak symbols, archives searched in order, and symbol versioning. We deliberately have none, so this lesson is only the core idea.

The next lessons need the answer as a location, so the result provides `Target(obj, sym)`: for a symbol of an object, which object, section and offset it ends up at. A definition in the same object resolves to itself; an undefined reference resolves through the global table.

The object parser from the previous lesson is provided, and the tests build objects with their own independent writer.
