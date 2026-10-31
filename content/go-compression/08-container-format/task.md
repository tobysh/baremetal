Implement `Compress` and `Decompress` in `starter.go`. The starter provides `Pack`/`Unpack`, `AppendUvarint`/`Uvarint` and the error values.

- `Compress(data, chunk)` writes the version 1 layout above. Empty input gives the header and the end marker. The exact bytes are compared with a file written by an independent Python implementation.
- `Decompress(file, maxOutput)` follows the reader rules above. A `maxOutput` equal to the data length must pass, one less must give `ErrTooLarge`.

Hidden tests: known files, round trips, all proper prefixes, all single-bit flips, header cases, hand-built chunks that lie about `rawLen` (3 GB claims) or `encLen` (4 GB claims) while **allocation is measured**, a bad CRC, and a maxOutput boundary.