Read the line with `char line[1024]; fgets(line, sizeof line, stdin);`. Remember the text ends with `\n`, which is not a letter so it passes through unchanged. When printing, strip it or print with `%s` and adjust the output yourself.
===
Write `static int score(const char *s, int shift)`: for each letter, decrypt it with `(c - base + 26 - shift) % 26`, and add 1 when the result is one of `"etaoinshr"` (use `strchr` on the lowercase letter).
===
Loop `shift` from 1 to 25, remember the one with the highest score, then print `shift: %d` and the decrypted text, character by character.
