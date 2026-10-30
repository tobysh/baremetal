`set_name(record *r, const char *s)` stores `s` in `r->name`, which holds 8 bytes.

- Copy at most 7 characters, then write the terminating `'\0'`, so `name` is always a valid string.
- Return the number of characters of `s` that did **not** fit (`0` if everything fit).
- Never write outside `name`: both guards must stay `0xAA`.
- Leave the unused bytes of `name` alone; only the copied characters and the terminator are written.

Several different lengths are tried, including exactly 7 and 8 characters and a very long string.
