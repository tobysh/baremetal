---
title: Infer an unknown record format
summary: Work out the layout of an undocumented binary log format from three annotated sample dumps, then write a strict parser for it.
minutes: 55
difficulty: hard
tags: security, reverse-engineering, formats, parsing
id: sec-reverse-engineering/infer-log-format
mode: stdout
---
You are handed some bytes from a made-up logging tool, `rlog`, with no documentation. All you have are three sample files and the text a viewer printed for each. This is the usual situation with proprietary or legacy formats, and the method is always the same: line up the bytes with what the viewer says they mean, find the anchors (magic numbers, small counts, obvious text), and form a field-by-field theory, then test it on the next sample.

Sample 1 (viewer output below it):

```
52 4c 01 02 00 01 3c 00 00 00 04 62 6f 6f 74 02 3d 00 00 00 08 64 69 73 6b 20 39 31 25

60 INFO boot
61 WARN disk 91%
records: 2
```

Sample 2:

```
52 4c 01 01 00 03 03 02 01 00 0b 66 61 6e 20 73 74 61 6c 6c 65 64

66051 ERROR fan stalled
records: 1
```

Sample 3:

```
52 4c 01 02 00 09 05 00 00 00 01 3f 00 ff ff ff ff 05 01 74 61 62 7f

5 L9 ?
4294967295 DEBUG .tab.
records: 2
```

Some clues to start. `52 4c` is the text `RL`. The text bytes are plainly visible: `62 6f 6f 74` is `boot`. Before each text there is a byte that equals the text length (`04` before `boot`, `0b` = 11 before `fan stalled`). The number `60` printed for the first record is `3c` in hex, and in sample 2 the number `66051` is `0x010203`, which appears in the file as `03 02 01 00`. That tells you the byte order, and the field size. Comparing samples 1 and 3 shows which byte holds the level, and sample 3 shows what the viewer does with a level it has no name for, and with bytes that are not printable text.

Work the layout out yourself. What you cannot infer from samples alone, because a viewer only shows good files, is how the tool should behave on damaged input, so that part is specified in the task. A parser for an unknown format should be strict: never read past the end of the data and say clearly what is wrong. Remember too that an inferred format is a *model*: it matches your samples, and a real tool may have rules you have never seen. State that when you report it.
