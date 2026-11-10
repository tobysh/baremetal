Start with `fnv1a64`: `h = (h ^ byte) * 0x100000001b3ULL` for each byte, with `h` a `uint64_t`. Check the empty input and the single byte `a` (`af63dc4c8601ec8c`) before moving on.
===
For each item, walk the manifest line by line. Parse a line with `sscanf("%llx %255s", ...)` into an `unsigned long long` and a name buffer, and compare the name with `strcmp` for an exact match.
===
Move to the next line with `strchr(p, '\n')`; when there is none you are on the last line, so stop after handling it. Do not assume the final line ends with a newline.
===
Decide the status in this order: name not found gives 2, then recompute the digest and compare for 0 or 1. Count nonzero statuses as you go and return that count.
