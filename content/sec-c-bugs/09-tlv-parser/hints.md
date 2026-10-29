Check `len - pos < 3` before reading the three header bytes, and `l > len - pos` before skipping the value.
===
Set both outputs to zero at the top, and only assign them on the return-1 path.
