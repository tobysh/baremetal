The helpers `Wrap`, `LoadCapsule` and the generator `Rand` (splitmix64, `Next() uint64`) are supplied. The capsule layout is `BML1`, entry (u16 LE), code length (u16 LE), CRC32 of the code (u32 LE), then the code. Implement:

```go
func SeedFor(name string, index int) uint64
func Mutants(valid []byte, seed uint64, n int) ([][]byte, error)
func Minimize(input []byte, fails func([]byte) bool) ([]byte, error)
```

**SeedFor**: the 64-bit FNV-1a hash (offset 14695981039346656037, prime 1099511628211) of the bytes of `name` followed by `uint64(index)` as 8 little-endian bytes.

**Mutants**: `valid` must be at least 12 bytes and `1 <= n <= 512`, otherwise return `nil, error`. Use `r := Rand{seed}` as the only source of randomness and return `n` mutants, each a fresh copy that differs from `valid` (never modify `valid`). For each mutant choose a kind with `r.Next() % 6`:

0. flip one random bit anywhere;
1. truncate to a random shorter length (0 to len-1);
2. append 1 to 8 random bytes;
3. replace one random byte of the first 12 (header) bytes with a different value;
4. replace one random code byte (index >= 12) with a different value, then recompute the code length field and CRC32 so the header is consistent;
5. insert a random byte into, or delete a byte from, the code (keep at least one byte), then recompute length and CRC.

When the code part is empty, kinds 4 and 5 fall back to kind 0. If a mutant equals `valid`, draw again (up to 8 tries, then append a zero byte). The same seed gives the same list; different seeds give different lists. Hidden tests check these properties (determinism, no input aliasing, a healthy share of length-changing mutants and of header-consistent mutants, plenty of distinct mutants) rather than exact bytes.

**Minimize**: if `fails(input)` is false return `nil, error`. Otherwise return a byte slice for which `fails` is still true and where removing any single byte makes it false. Never modify `input` and pass `fails` a fresh copy each call (the predicate may scribble on it). Stay within roughly `n*n` calls for an `n`-byte input.
