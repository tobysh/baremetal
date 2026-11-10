Read one line at a time with `fgets`, remove the line ending, and count the line number. Split on `|` by hand with `strchr` or a loop: you need exactly four fields, so a fifth bar or a missing bar means `malformed`.
===
Validate the format before anything else: `SEQ` all digits with no leading zero, actor and action non-empty, hash exactly 16 characters from `0-9a-f`. Only then convert `SEQ` with `strtoul` and compare it with the expected number.
===
To hash, build `PREV|SEQ|ACTOR|ACTION` into a buffer with `snprintf` using the stored previous hash text (sixteen zeros at the start). Run `h = (h ^ byte) * 1099511628211ULL` over every byte starting from 14695981039346656037, and print with `%016llx`.
===
Keep `prev` as the previous line's stored hash and `expected` as the next sequence number; update both only after a line passes all three checks, and return as soon as one fails. An empty input prints `chain ok: 0 entries head=0000000000000000`.
