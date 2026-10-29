Use a pointer `p` that walks the line and an output index `o` into `buf`. The outer loop skips separators, then starts a word by setting `argv[n] = buf + o`.
===
Inside a word, loop while `*p` is not a space, tab or `'\0'`. Look at `*p`: if it is `'\''`, step past it and copy characters until the next `'\''`. If it is `'"'`, do the same but treat `\"` and `\\` specially. Otherwise it is a plain character (and a backslash makes the next one literal).
===
If the quote loop ends because `*p == '\0'`, the quote was never closed: return `-1`. Write a tiny helper (or macro) `put(c)` that returns `-2` when `o >= buf_size`, and call it for every literal character and for each terminating `'\0'`.
