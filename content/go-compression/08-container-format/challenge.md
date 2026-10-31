**Challenge: BMZ version 2, without breaking version 1.**

Version 2 files look like this:

```
"BMZ" 0x02
uvarint flags
[uvarint metaLen, meta bytes]      only if flags has FlagMeta (0x02)
chunk* 0x00                        same layout as version 1
```

Chunk bodies now start with a method byte: `0` stored, `1` RLE, `2` Huffman (`HuffEncode` output follows the method byte).

- `CompressV2(data, chunk, meta)` always sets `FlagHuffman` (0x01), and `FlagMeta` when `meta` is non-empty. For each chunk it writes the **smallest** of the three methods; ties go to the lower method number. The bytes are compared with a file made by an independent implementation.
- `DecompressAny(file, maxOutput)` reads version 1 **and** version 2. Rules for version 2: flag bits 0x0C are optional and ignored; any other unknown flag bit (0x10 and above, including a flags varint above 255) is `ErrVersion`; metadata larger than 1024 bytes is `ErrTooLarge` and is skipped, never interpreted; a Huffman chunk when `FlagHuffman` is not set is `ErrCorrupt`; a Huffman chunk's declared symbol count must be checked against its `rawLen` before decoding. All earlier size, prefix and bit-flip properties still hold. Versions other than 1 and 2 are `ErrVersion`.
- `Compress` must keep writing byte-identical version 1 files, and the **old reader** (`Decompress`) must answer `ErrVersion` for a version 2 file instead of misreading it. That is the compatibility design: the version byte is the one thing an old reader always understands.

Whether *optional-versus-critical flag bits* is a good design is a judgement the tests cannot make; they only check the stated rules. Compare it with always bumping the version number.
