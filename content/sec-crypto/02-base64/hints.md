Process the input three bytes at a time. Combine them into one 24-bit number: `(in[i] << 16) | (in[i+1] << 8) | in[i+2]`, then take four 6-bit pieces with `>> 18`, `>> 12`, `>> 6` and no shift, each `& 63`.
===
Keep the alphabet in a string: `static const char T[] = "ABC...xyz0123456789+/";` and use each piece as an index into it.
===
For the last group, when `n - i` is 1 or 2, treat the missing bytes as 0 for the maths, but write only 2 or 3 real characters and then `=` characters until you have written 4. Remember the final `'\0'`.
