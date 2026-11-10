Handle the 12-byte header first: `n >= 12` and QDCOUNT (`be16(p + 4)`) equal to 1. Keep an `off` variable starting at 12 for the walk through the name.
===
In the name loop, read one length byte (after checking `off < n`). Zero ends the name. A length of 64 or more means a pointer or reserved bits, so return -1. Then check `len <= n - off` before reading the label bytes.
===
Track the output length `k` separately from `off`. Before copying a label, add a dot if `k > 0`, and check that the dot plus the label still fit within 253 characters. Validate each byte as a letter, digit or hyphen while copying.
===
After the loop, if `k` is still 0 store `.`. Then require `n - off >= 4` before reading type and class. Build the answer in a local `Question` and assign it to `*out` only at the end.
