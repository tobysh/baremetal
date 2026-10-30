Implement `LoadCapsule(blob []byte) (Image, error)`. Apply all capsule and instruction-stream rules below, including the 64-instruction limit. Return `Image{}` and an error on any violation. Validate CRC32 before decoding. Entry must be an instruction boundary, including a possible final HALT; return its **instruction index** in Image.Entry. Return all decoded instructions in Image.Instructions, including any prefix before entry. Preserve input and make returned text independent of the byte buffer.

The capsule format is invented for this course; it is distinct from the analysis course's BMX files.

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 4 | ASCII magic `BML1` |
| 4 | 2 | entry **byte offset** into code, little-endian uint16 |
| 6 | 2 | code byte length, little-endian uint16 |
| 8 | 4 | IEEE CRC32 of code bytes, little-endian uint32 |
| 12 | length | code bytes |

Code length is 1–4096; entry must be smaller than length. No trailing bytes are allowed. CRC32 detects accidental corruption; an attacker can recompute it, so it does not establish authenticity.

The instruction stream uses these opcodes:

| Byte | Instruction | Following bytes |
| --- | --- | --- |
| 0 | HALT | none; exactly one, at the end |
| 1 | EMIT | one-byte length, then that many text bytes (0–255) |
| 2 | READ | one-byte length, then a nonempty capability key (1–255 bytes) |
| 3 | WAIT | nonzero uint16 virtual ticks in little-endian order |

Text is a Go byte string; embedded NUL and non-UTF-8 bytes are valid. There are no branches or host addresses. Programs have 1–64 instructions, ending with HALT and no earlier HALT. Fields unused by an opcode must be zero/empty. WAIT changes a counter rather than sleeping.

For code `EMIT "hi"; WAIT 258; HALT`, byte offsets are 0, 4 and 7. Entry 4 maps to instruction index 1. Entry 2 is invalid because it points inside the text operand.

