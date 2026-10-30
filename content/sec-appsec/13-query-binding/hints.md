First count placeholders (outside literals) and validate the arguments; only then start writing.
===
Write through a helper that refuses to store a byte when `len + 1 >= cap`, and remember to set `out[0] = 0` when failing.
