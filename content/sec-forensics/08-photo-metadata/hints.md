Write one helper `word(p, bytes, big)` that builds a 2 or 4 byte number from either byte order. Then read the header and decide the order from the first two bytes.
===
Validate in layers: header and `n >= 8`, directory offset and the 2-byte count, then `count <= (n - off) / 12` before touching any entry. Rewrite every range test as `offset <= n && length <= n - offset`.
===
For an ASCII entry use the value field itself when `count <= 4`, otherwise treat it as an offset. Insist that the last of the `count` bytes is zero and that there is no earlier zero, then copy exactly `count` bytes into the field.
===
For `to_unix` first check the shape (digits and separators at fixed positions), then the ranges. Add 365 or 366 days for each year since 1970, then the months before this one (February has 29 days in a leap year), then `day - 1`, and finally hours, minutes and seconds.
