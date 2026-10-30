Implement `Link(objs []Object) ([]byte, error)`.

```go
type Reloc struct {
	Site   int    // byte offset in this object's code of a CALL's 2-byte operand
	Symbol string // exported name the operand must point at
}
type Object struct {
	Code    []byte
	Exports map[string]int // name -> byte offset of an instruction in Code
	Relocs  []Reloc
}
```

Opcodes: HALT 0, EMIT 1 (length byte, text), READ 2 (length byte, nonempty key), WAIT 3 (nonzero uint16 ticks, little endian), CALL 4 (uint16 operand). A CALL operand inside an object must be `00 00`.

Rules. Return `nil, error` for any violation and never modify `objs`.

1. 1–8 objects. Each object's code is non-empty, decodes completely as instructions (operands inside the code, no unknown opcode) and its **last instruction is HALT**. HALT may also appear earlier.
2. Linked code is at most 4096 bytes and at most 64 instructions in total.
3. Export names are 1–32 bytes. Each export offset must be the start of an instruction in its own object (inside an operand or out of range is an error). The same name exported by two objects is an error.
4. Every relocation `Site` must be exactly the operand of a CALL in the same object (so site-1 is a CALL instruction start). Out-of-range or negative sites, sites inside another operand and the CALL opcode byte itself are errors. Two relocations for one site are an error.
5. Every CALL must have exactly one relocation (otherwise it is unresolved). Every relocation `Symbol` must be exported by some object (any object, including its own).
6. A symbol named `main` must be exported; the capsule entry is its linked offset.
7. Patch each CALL operand with the symbol's linked absolute offset, little endian.
8. Emit `BML1` when the linked code contains no CALL and exactly one HALT, otherwise `BML2`. Header: 4 magic bytes, entry (uint16 LE), code length (uint16 LE), CRC32 IEEE of code (uint32 LE), then code.

The result must not alias input slices.

Example: object A = `CALL f; HALT` (`04 00 00 00`) with export `main`=0 and relocation Site 1 -> `f`; object B = `EMIT "hi"; HALT` (`01 02 68 69 00`) with export `f`=0. B's base is 4, so the CALL operand becomes `04 00` and the capsule is `BML2` with entry 0 and code `04 04 00 00 01 02 68 69 00`.
