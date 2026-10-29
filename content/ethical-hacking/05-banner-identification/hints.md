Check the prefix of the banner first, then write one small parsing helper per protocol. Copy into the output with a helper that takes the capacity, writes at most `cap - 1` characters and always terminates.
===
For HTTP, walk the lines. A header line starts with `Server:`, compared case-insensitively. Skip spaces after the colon and stop the value at CR, LF or end of string.
===
For `ver_cmp`, parse a number with a loop, skip the dot, repeat. Loop until both strings are finished so `1.0` and `1.0.0` compare equal.
