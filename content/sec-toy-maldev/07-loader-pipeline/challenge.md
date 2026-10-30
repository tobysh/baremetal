**Track challenge: admit only a trusted, policy-compliant artifact.**

A checksum-valid capsule is not necessarily an approved payload. Add a loader admission gate that verifies identity and audits the **entire** image, including instructions before entry.

Keep the supplied helpers and implement:

```go
func AdmitBundle(bundle []byte, caps map[string]string,
                 expectedDigest string, maxOps int,
                 maxTicks uint32, maxOutput int) (Trace, error)
```

1. Unpack the envelope. Compute SHA-256 of the complete decoded capsule, including its BML1 header, and require exact equality to expectedDigest encoded as 64 lowercase hexadecimal characters. The expected value is trusted caller input. Hash the capsule, not the XOR envelope; two envelopes with different XOR keys can represent the same approved artifact.
2. Load and validate the capsule normally.
3. Audit the whole instruction list from index zero, not only the entry suffix. Every READ must be present in caps; the **whole image** must fit maxOps, maxTicks and the runtime's 4096-byte output limit. The supplied Run helper can evaluate this audit because it only builds an in-memory trace.
4. Count every output byte in that whole-image audit, including repeated reads, and require the total <= maxOutput. A negative maxOutput is invalid; zero permits only zero output bytes.
5. Execute and return only the entry suffix using the same capabilities and operation/tick budgets. Prefix outputs and reads must not appear in the returned trace.

On any error return Trace{},error and preserve inputs. This is a conservative, linear whole-image policy, not general control-flow analysis. The format has no jumps and exactly one final HALT.

Example: a capsule starts with READ `restricted`, then entry points at EMIT `ok`; HALT. The base capstone can execute it with no read capability, since the prefix is skipped. This challenge must reject it until `restricted` is explicitly permitted, even though that instruction is not executed. Another capsule with a long skipped WAIT must satisfy the whole-image delay budget.

**Why this is harder:** integrity, identity and permission are different checks. CRC32 can be recomputed after changing an artifact. A SHA-256 digest identifies bytes only when the expected digest comes from a trusted source; it does not authenticate an untrusted manifest by itself. Auditing only the chosen entry point can overlook code retained elsewhere in the image.

**What the tests establish:** different envelopes for the same artifact, changed capsules with recomputed CRCs, digest mismatch, unauthorized skipped reads, prefix resource consumption, byte-counted output bounds and returned traces containing only executed instructions. The challenge remains entirely within the inert bytecode machine.
