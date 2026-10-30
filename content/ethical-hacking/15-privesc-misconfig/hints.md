Store the F/D entries in an array and write two helpers: `lookup(path)` and `guest_can_write(entry)`. Keep guest's groups in a small list filled from the G lines (these may appear anywhere in the input, so read everything first).
===
Do the S/C lines in input order after reading all input, then loop over the F entries for setuid. The parent directory is the path up to the last slash. Use `%04o` so that `0755` prints with a leading zero.
