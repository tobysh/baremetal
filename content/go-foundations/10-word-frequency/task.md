Implement `TopWords(r io.Reader, n int) []WordCount`, where `WordCount` has the fields `Word string` and `Count int`.

- Words are runs of letters and apostrophes; everything else separates words. Compare case-insensitively and return words in lower case (`"Don't"` and `"don't"` are the same word).
- Return at most `n` entries, most frequent first, ties broken alphabetically.
- If `n <= 0` or there are no words, return an empty (non-nil) slice.
