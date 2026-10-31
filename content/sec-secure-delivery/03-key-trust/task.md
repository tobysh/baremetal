Implement the two methods on `*Store` (types, errors and `SigningInput` are supplied):

- `Verify(env Signed, role string, now int64) error` applies the checks in the order listed in the lesson. The key is found by `env.KeyID` in `s.Keys` (first match), the signature is checked with that stored key only.
- `ApplyRotation(env Signed, now int64) error` verifies `env` with role `root` at `now`, parses the payload strictly (`ErrBadRotation` for any malformed line, an empty payload, or a payload without a final newline), applies every line, and changes the store only if every line is valid. `add` with an id already in the store (or repeated in the same payload) is `ErrDuplicateKey`. A `revoke` may name a key added earlier in the same payload.

`main` builds a store with one root and one release key, verifies a release, then rotates the release key. Hidden tests cover wrong roles, expired and not-yet-valid keys (boundaries are inclusive at `NotBefore`, exclusive at `NotAfter`), revoked keys, a release key trying to rotate, a message re-labelled with a different key id, and rotations that fail halfway and must leave the store untouched.
