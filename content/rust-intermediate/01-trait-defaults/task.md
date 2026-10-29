Complete the `Scorer` trait and its three implementors.

- Default `fn total(&self, words: &[&str]) -> u32`: the sum of `score` over all the words.
- Default `fn best(&self, words: &[&str]) -> Option<usize>`: the index of the highest-scoring word, the **first** one on a tie, or `None` for an empty slice.
- Default `fn label(&self) -> String`: `"scorer:"` followed by `Self::NAME`, for example `scorer:length`.
- `LengthScorer` (`NAME = "length"`): the score is the number of bytes in the word.
- `VowelScorer` (`NAME = "vowel"`): the score is the number of vowels `a e i o u`, upper or lower case.
- `WeightedScorer { weight }` (`NAME = "weighted"`): the score is the word's length times `weight`.

Hidden tests try many different words and weights.
