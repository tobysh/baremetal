Use `strings.CutPrefix(s, "sha256:")`, then check the length is exactly 64 before decoding.
===
`hex.DecodeString` accepts upper case, so check lower case yourself: every byte must be `0-9` or `a-f`. Do that before decoding.
===
Compare with `bytes.Equal` or `subtle.ConstantTimeCompare` on the decoded 32 bytes, not on strings.
===
For the stream: `h := sha256.New()`, then `n, err := io.Copy(h, io.LimitReader(r, maxSize+1))`. Reading one byte more than the limit is how you tell "exactly maxSize" from "more": if `n > maxSize` return `ErrTooLarge`. Return `err` if it is non-nil.
