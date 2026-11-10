Handle one row at a time: offset `off`, then `m = min(16, n - off)` bytes. Format the row into a local array first so you can measure it before deciding what to copy.
===
Build the hex field separately. Put a space before every byte whose index within the row is even and nonzero, then pad with spaces until the field is 39 characters. A short last row must be padded the same way.
===
Compute the printable view with a plain comparison (`>= 0x20 && <= 0x7e`), not `isprint`. Then format the row as `"%08zx: %s  %s\n"`.
===
Copy the finished row character by character: always add to the total, but store a character only while `total < cap - 1`. Terminate once at the end when `cap > 0`, and keep the return value independent of how much was stored.
