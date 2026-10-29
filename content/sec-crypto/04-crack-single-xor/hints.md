Write a helper `static long score(const uint8_t *p, size_t n)` that you call on the decrypted bytes. Start simple: +10 for a space, +8 for `e`/`t`/`a`/`o`/`i`/`n` (either case), +4 for other letters, -50 for non-printable bytes.
===
Make a copy of the decrypted text in a local buffer, or compute the score on the fly with `cipher[i] ^ k`. Score all 256 keys and remember the best. Use `long` for scores so a penalty can go negative.
===
If one key is close, refine the scorer: give a letter weight by its place in `"etaoinshrdlcumwfgypbvkjxqz"` (earlier = higher). Use `strchr` on the lowercase letter.
