Parse in this order: `Uvarint` n, `Uvarint` k, check `k <= 256`, check `len(data)-pos >= 2*k`. Map `ErrShort` to `ErrTruncated` and other varint errors to `ErrCorrupt`.
===
Inside the pair loop reject `symbol <= previous`, `length < 1`, `length > MaxCodeLen`. Add `1 << (48-length)` to a Kraft sum. After the loop: `k == 0` needs `n == 0`; `k == 1` needs length 1; otherwise the sum must be exactly `1 << 48`.
===
For decoding without a tree: canonical codes of one length are consecutive. For each length keep `first[l]` (the first code), `count[l]` and `offset[l]` (index into the symbols sorted by length then value). A `code` of `l` bits is a symbol when `first[l] <= code < first[l]+count[l]`.
===
Read bit by bit, `code = code<<1 | bit`, `l++`, and test. More than 48 bits without a match: `ErrCorrupt`. Ran out of bits: `ErrTruncated`.
===
After `n` symbols the reader must be within the last byte (`(Pos()+7)/8 == len(payload)`) and the remaining bits must all be zero.
