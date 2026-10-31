Implement `ParseExe` and `Describe` in `starter.go`.

`ParseExe(b)`: header checks in order: fewer than 12 bytes `ErrTruncated`; magic `ErrMagic`; version 1 `ErrVersion`; segment count 1..4 and reserved bytes zero `ErrCorrupt`; the segment table must fit in the file (`ErrTruncated`). Then each segment, in order, using the checks listed in the starter comment (flags, W with X, sizes and address, content inside the headers, content past the file, overlap with an earlier segment). Finally the entry must be inside an executable segment, `ErrEntry`. Copy each segment's file bytes into `Data`.

`Describe` prints exactly:

```
TEXE entry=0x00001004 segments=2
seg0 R-X 0x00001000-0x00001014 file=20 zero=0
seg1 RW- 0x00001100-0x00001118 file=8 zero=16
```

The address range is `[addr, addr+memsz)`; `zero` is `memsz-filesz`; permissions print as `R`, `W`, `X` or `-` in that order. Every line ends in a newline. Hidden tests parse executables built by an independent linker model, flip fields one at a time, cut the file at every length and compare `Describe` output with fixed text.
