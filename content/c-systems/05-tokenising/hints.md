For `split_words`, use an index `i`. Skip spaces with `while (s[i] == ' ') i++;`, stop if you reach `'\0'`, then remember `start = i` and advance `i` until a space or `'\0'`.
===
Copy `min(length, 15)` characters into `words[count]`, add the `'\0'`, and increase `count`. Make the loop stop when `count == max_words`.
===
For `parse_int`: handle an optional sign first, then require at least one digit. Loop over the rest; if any character is not between `'0'` and `'9'` return 0. Build the value with `value = value * 10 + (c - '0')`, and negate at the end if the sign was `-`. Only write to `*out` once you know the whole string is valid.
