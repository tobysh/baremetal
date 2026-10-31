---
title: Inspecting an Executable
summary: Parse a TEXE executable with W^X, overlap and entry-point checks, then print a readelf-style description of its segments.
minutes: 40
difficulty: medium
tags: binary-formats, loader, segments
id: go-executable-formats/inspecting-an-executable
mode: harness
---
A linker's output is an **executable**: no names, no relocations, just *segments* to place in memory and an *entry point* to jump to. This lesson reads one, the way `readelf` or a loader does, before we ever write one.

## Outcome

`ParseExe` accepts valid executables and rejects each class of malformed one with a specific error; `Describe` prints a stable, human-readable report.

## What a loader must check

Our TEXE format has a 12-byte header (magic, version, segment count 1 to 4, entry address) and 20-byte segment entries: permission flags, virtual address, file offset, file size and memory size. The memory size may exceed the file size; the extra bytes are zero, which is how **bss** costs no file space.

Before mapping anything a loader asks:

- Are the permissions sane? A segment must be readable, and **never writable and executable at once** (`ErrPerm`). That "W^X" rule stops data being run as code.
- Do the sizes fit? File bytes cannot exceed memory bytes, the address range must not wrap past 32 bits or beyond the 1 MiB address limit, and the first page (below `0x1000`) stays unmapped.
- Do segments **overlap** in memory (`ErrOverlap`)?
- Does the **entry point** lie inside an executable segment (`ErrEntry`)? An entry in data would make the machine run bytes that were never meant to be code.
- Does the file contain what the table promises?

Nothing here has to do with real Linux or Windows loaders; it is the same family of checks in a smaller format. Passing them also does not make the *program* safe; it only shows the file is well formed.

The format is **invented for this course** (named TOBJ for objects and TEXE for executables). It borrows ideas from ELF and a.out but is far smaller, so every byte can be accounted for. It shows how linkers work, not how any real platform loads programs. Real formats add alignment rules, dynamic linking, position independence and much more.
