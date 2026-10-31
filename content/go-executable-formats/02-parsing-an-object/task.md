Implement `ParseObject(b []byte) (*Object, error)`. Use `ParseHeader`, then read the tables. Rules (every violation wraps an `Err` value; use the one named):

**Names.** A name offset must be inside the string table and the name must end with a NUL inside it: `ErrCorrupt`.

**Sections** (16 bytes: name offset, kind, 3 zero padding bytes, file offset, size). Kind must be 1 text, 2 data or 3 bss and the padding zero: `ErrCorrupt`. Text and data contents must start at or after the end of the tables (`ErrCorrupt` otherwise) and end inside the file (`ErrTruncated` otherwise); copy them into `Data`. Bss has file offset 0 and size at most `maxBss`: `ErrCorrupt`; its `Data` stays nil.

**Symbols** (12 bytes: name offset, section u16, bind, zero padding byte, value). Bind is 0 local or 1 global. Section `0xFFFF` means undefined (`Sec = -1`); an undefined symbol must be global with value 0. A defined symbol needs an existing section and a value at most the section size (a value equal to the size is allowed). Global symbols need a non-empty name. All violations: `ErrCorrupt`.

**Relocations** (16 bytes: section u16, kind, zero padding byte, offset, symbol u16, reserved u16 (zero), addend i32). Kind 1 or 2. The section must exist and be text or data, `offset+4` must not exceed its size, the symbol index must exist: `ErrCorrupt`.

Bytes after the last content are ignored.
