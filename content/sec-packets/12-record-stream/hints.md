Keep `pos` as the offset of the next header, and compare `n - pos` with 5 and with `5 + length` instead of adding to `pos` first.
===
Check the header fields before looking at the body, so an invalid header is `-1` even when the body has not arrived.
