Compare `n` with `cap` before calling `memcpy`.
===
Only require non-NULL pointers when `n != 0`. Return before any copy on failure.
