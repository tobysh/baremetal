Test `len < 0` before any cast to `size_t`.
===
After the sign check, compare the converted length to `cap`, then validate pointers and copy.
