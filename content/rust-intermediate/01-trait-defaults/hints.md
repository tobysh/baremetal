`total` is a one-liner: `words.iter().map(|w| self.score(w)).sum()`.
===
For `best`, loop with `enumerate()`, keep the best `(index, score)` so far, and replace it only when the new score is strictly greater. That keeps the first of any tie.
===
`label` is `format!("scorer:{}", Self::NAME)`. `VowelScorer::score` can use `word.chars().filter(|c| "aeiouAEIOU".contains(*c)).count() as u32`.
