`format_time`: split into `h = total / 3600`, `m = total % 3600 / 60`, `s = total % 60`, and write them with `"%02d:%02d:%02d"`. Check the return value of `snprintf` against `cap`.
===
`format_pairs`: keep `size_t used = 0;` and in each turn call `snprintf(buf + used, cap - used, "%s=%s;", ...)`. If the return value is negative or `>= cap - used`, return `-1`. Start by setting `buf[0] = '\0'` so `n == 0` gives an empty string.
===
`parse_pairs`: use `enum { IN_KEY, IN_VALUE }`, plus `klen`, `vlen` and `count`. In `IN_KEY`: `=` finishes the key (error if `klen == 0`), `;` is an error (no `=`), anything else is added to the key (error if `klen == 15`). In `IN_VALUE`: `;` finishes the pair, `=` is an error, anything else is added to the value.
===
Finish a pair by adding the `'\0'` terminators, checking `count < max`, and bumping `count`. After the loop: if you are in `IN_KEY` with `klen > 0`, the key had no `=`, so return `-1`. If you are in `IN_VALUE`, finish the last pair (the final `;` was optional). If you are in `IN_KEY` with `klen == 0`, you are done.
