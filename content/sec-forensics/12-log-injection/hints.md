Handle one input byte at a time and decide its escaped form first: a plain byte is 1 character, `\n`, `\r` and `\\` are 2, and other control bytes are 4. Build that piece in a small local array so you know its length before you copy anything.
===
Keep two counters: `total`, which always grows by the piece length, and `w`, the number of characters actually stored. The return value is `total`, however little of it was stored.
===
Store a piece only if it fits whole together with the terminator: `w + len < cap`. Use an unsigned `char` for the byte, so `0x80` and above are not treated as negative.
===
Add a `stopped` flag. The first time a piece does not fit, set it and store nothing more, even for later shorter pieces. At the end terminate with `out[w] = 0` when `out` is not NULL and `cap > 0`.
