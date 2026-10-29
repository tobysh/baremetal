Implement `Bloom`, with the sizing and serialisation from the lesson.

- `NewBloom(n, bitsPerKey int) *Bloom` allocates `m = max(64, n*bitsPerKey)` bits rounded up to a multiple of 64, and `k = round(bitsPerKey * 0.69)` clamped to `[1, 30]`. It must not panic for `n == 0`.
- `(*Bloom) Add(key []byte)` and `(*Bloom) MayContain(key []byte) bool`. Anything you added must always report `true`. On an empty filter every key reports `false`. Use the double-hashing scheme with FNV-1a from the lesson (or any scheme with the same behaviour).
- `(*Bloom) Marshal() []byte` gives `k` (1 byte), `m` (4 bytes, big-endian), then all words, each as 8 bytes big-endian.
- `UnmarshalBloom(data []byte) (*Bloom, error)` is the inverse. It returns an error if the data is too short, if `k` is outside `1..30`, if `m` is zero, or if the length does not match `m` exactly.

The tests add 10,000 keys at 10 bits per key and require a false-positive rate under 3% on 20,000 keys that were never added.
