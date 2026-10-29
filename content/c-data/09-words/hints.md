`count_words`: keep an `in_word` flag. A space sets it to 0. A non-space when the flag is 0 starts a new word: set the flag and add 1 to the count. `longest_word` reuses the idea: a `current` length that grows on each non-space and resets to 0 on a space, and a `best` that remembers the largest `current`.
===
For `caesar`, first reduce the shift to 0..25 with `((shift % 26) + 26) % 26`, which also fixes negative shifts. Then for a lowercase letter: `'a' + (s[i] - 'a' + k) % 26`. Do the same with `'A'` for capitals, and skip everything else.
===
```c
void caesar(char s[], int shift) {
    int k = ((shift % 26) + 26) % 26;
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] >= 'a' && s[i] <= 'z') {
            s[i] = (char)('a' + (s[i] - 'a' + k) % 26);
        } else if (s[i] >= 'A' && s[i] <= 'Z') {
            s[i] = (char)('A' + (s[i] - 'A' + k) % 26);
        }
    }
}
```
