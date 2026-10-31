Write a small `find(id) *TrustedKey` helper. Check the lookup, `s.Revoked[id]`, `k.Role != role`, and `now < k.NotBefore || now >= k.NotAfter` in that order, and only then call `ed25519.Verify(k.Pub, SigningInput(...), env.Sig)` (after checking the key length so it cannot panic).
===
For `ApplyRotation`, verify first. Then work on a **copy**: a new slice of keys and a new revoked map. Only when every line succeeded assign the copies back to `s`.
===
Parse each line with `strings.Split(line, " ")`. `add` needs exactly 6 fields, `revoke` exactly 2. Roles are only `root` or `release`. Use `strconv.ParseInt(f, 10, 64)` for the times, require `notBefore < notAfter`, and `hex.DecodeString` plus a 32-byte length check for the key.
===
Revoking marks the id in `Revoked`; it need not remove the key. Revoking an id that is not in the (copy of the) store is `ErrBadRotation`.
