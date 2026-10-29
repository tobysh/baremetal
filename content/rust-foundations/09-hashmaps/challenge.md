**Challenge: the most common words.**

Write `fn top_words(text: &str, n: usize) -> Vec<(String, usize)>` that returns the `n` most frequent words with their counts.

- Count words exactly as `word_count` does: split on whitespace, lowercase, strip ASCII punctuation from both ends, skip empty words. (The starter has a `word_count` for you to reuse.)
- Sort by count, highest first. Words with the same count go in alphabetical order, so the answer is always the same.
- If there are fewer than `n` distinct words, return all of them. `n == 0` gives an empty vector.
- Words are compared as the lowercase text, so `"Don't"` and `"don't"` are one word, and `"don't"` keeps its inner apostrophe.
- Remember that iterating over a `HashMap` gives a different order on every run, so you must sort before you return.
