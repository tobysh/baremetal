The instruction stream uses these opcodes:

| Byte | Instruction | Following bytes |
| --- | --- | --- |
| 0 | HALT | none; exactly one, at the end |
| 1 | EMIT | one-byte length, then that many text bytes (0–255) |
| 2 | READ | one-byte length, then a nonempty capability key (1–255 bytes) |
| 3 | WAIT | nonzero uint16 virtual ticks in little-endian order |

Text is a Go byte string; embedded NUL and non-UTF-8 bytes are valid. There are no branches or host addresses. Programs have 1–64 instructions, ending with HALT and no earlier HALT. Fields unused by an opcode must be zero/empty. WAIT changes a counter rather than sleeping.

Implement `Assemble(program []Instruction) ([]byte, error)`. Reject all contract violations, including unknown opcodes, missing/early HALT, too many instructions, oversized text and nonzero unused fields. On error return nil,error; never modify program. Return freshly owned bytes on success.

Example: EMIT `hi`, WAIT 258, HALT encodes as hex `01 02 68 69 03 02 01 00`.

