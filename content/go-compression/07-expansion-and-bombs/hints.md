Parse one token at a time: work out `n` (output size) and `need` (bytes of input the token uses). Check `i+need <= len(enc)` first.
===
Compute `total := len(out)+n` and `consumed := i+need` and test the limits before touching `out`.
===
Only after the checks, append: literals copy `enc[i+1:i+need]`; repeats append `enc[i+1]` `n` times.
===
Pack: compare `len(rle) < len(data)` (strictly), not `<=`.
