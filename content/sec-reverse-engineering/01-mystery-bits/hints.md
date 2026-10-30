Write down the rule for inputs below 256 first, then decide what the second batch changes.
===
Build the result one bit at a time: bit `i` of the input becomes bit `7 - i` of the output, for `i` from 0 to 7.
===
Use `scanf("%u", &x)` in a loop and stop when it does not return 1.
