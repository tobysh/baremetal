Words are runs of non-space characters, separated by one or more spaces (the space character only). There may be leading or trailing spaces.

- `int count_words(char s[])` returns the number of words.
- `int longest_word(char s[])` returns the length of the longest word, or 0 if there are none.
- `void caesar(char s[], int shift)` shifts every letter forward by `shift` places in the alphabet, wrapping from `z` to `a` (and `Z` to `A`), keeping the case. Non-letters stay as they are. `shift` can be negative (shift backwards) or larger than 26.
