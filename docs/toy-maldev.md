# Toy Malware Development

`sec-toy-maldev` follows Offensive Infrastructure Lab on the security path. It adds seven Go lessons and a final optional challenge. Go Practice is assumed; Malware Analysis for Defenders is useful background. The existing analysis course remains focused on reading samples. This companion course teaches construction, loading and constrained execution of inert artifacts.

| Lesson | What learners build | What it teaches |
| --- | --- | --- |
| Payload bytecode | An assembler for EMIT, READ, WAIT and HALT. | Explicit operand lengths, byte order, binary zero bytes and instruction validation. |
| Payload capsule | A BML1 header with code length, entry byte offset and CRC32. | A loader's format contract and the difference between corruption detection and authenticity. |
| Reversible packaging | A BX01 envelope carrying a one-byte XOR key, decoded length and checksum. | Reversible representation, independent ownership of buffers and rejection of corrupt envelopes. |
| Validated loader | Full capsule validation and conversion of entry byte offsets to instruction indices. | Every operand and entry boundary must be checked, including instructions before entry. |
| Capability interpreter | A bounded interpreter returning an in-memory behavior trace. | Syntax does not grant permission; reads require explicit caller-supplied data, and execution consumes finite budgets. |
| Task protocol | A finite JSON echo/stop batch with session and sequence checks. | Replay-state handling, strict field parsing, trailing-data rejection and transactional error results. |
| Loader pipeline | Composition of unpacking, loading and interpretation. | Each layer enforces a different boundary; a valid outer envelope does not make an inner artifact valid or authorized. |

Every lesson has a compiling starter, reference solution, progressive hints and hidden tests. Each main function provides a small Run example. The capstone supplies the earlier helper implementations so the exercise focuses on composition. Public `sample.json` and `sample.bundle.hex` files live beside the capstone; the JSON is also included in the rendered task, so it can be used without an asset-download endpoint.

## The payload machine

The bytecode format is invented and does not contain native CPU instructions. EMIT returns text. READ retrieves a value from an explicit Go map. WAIT increments a virtual tick counter without sleeping. HALT ends execution. There are no branches, native addresses or instructions for process execution, injection, host filesystem access, networking or persistence. The existing runner remains unchanged.

A program has at most 64 instructions. Capsules contain at most 4096 code bytes. Packages contain at most 8192 decoded bytes. The interpreter additionally enforces caller-provided operation and virtual-time budgets, and a total output ceiling of 4096 bytes. Errors return no partial trace. These are lesson-format limits, independent of the runner's process and resource isolation.

The packaging key is stored openly in the envelope. XOR is not presented as encryption or detection evasion. CRC32 is an integrity check against accidental corruption, not a signature. Session labels and replay counters in the task lesson are not claimed to authenticate an untrusted sender. The task protocol is a function call over a finite JSON batch, with no live command channel or background loop.

## Final challenge: whole-artifact admission

The optional challenge on Loader Pipeline asks learners to admit only a caller-approved decoded capsule and audit the entire instruction list before returning the entry-point execution trace.

The expected SHA-256 digest is trusted input and covers the full decoded capsule, not its XOR wrapper. Multiple wrappers can therefore represent the same artifact. Recomputed CRCs do not bypass identity checks. The whole-image audit must enforce permitted reads, instruction count, virtual ticks and output bytes, including prefix instructions skipped by the entry point. The returned trace must still contain only the executed suffix.

This distinction is the reason for the challenge: structural validity, artifact identity and behavioral permission are separate requirements. A digest supplied by an attacker does not authenticate its own artifact. The linear whole-image audit is deliberately simple and is not a general control-flow analysis algorithm.

## Validation

Run `npm run verify:content -- sec-toy-maldev`, `npm run check:ids` and `npm run check:learning`. Hidden tests vary operands, data keys, byte strings, entry offsets, packaging keys and budgets. They also supply independent binary fixtures, corrupt nested layers, wrong-session and replayed task batches, rejected data reads, altered artifacts with recomputed CRCs and skipped instructions that violate policy.

The curriculum verifier checks that solutions pass, starters compile but fail, and the final challenge is not already solved by the base lesson solution. The usual same-process verdict limitations described in `docs/security.md` apply. No deployment, additional language runtime or change to sandbox privileges is required.
