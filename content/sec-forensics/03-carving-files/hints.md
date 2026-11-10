Keep one scan position `i`. At each position decide: is there a complete header here? If not, `i++`. If yes, look for the footer.
===
Check how many bytes remain (`n - i`) before comparing a header with `memcmp`. Start the footer search after the header, not at the header's last byte, so `ff d8 ff d9` is not accepted.
===
For PNG, search for `IEND` after the 8-byte signature and require 8 bytes from the start of `IEND` to the end of the evidence: loop while `n - j >= 8` rather than testing `j + 8 <= n`.
===
When an object is complete, store it only if `count < max`, then set `i = end`. Stop the outer loop when the output is full. When nothing is complete, just `i++` so a later header can still be found.
