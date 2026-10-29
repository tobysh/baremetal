`index_words`: `for (i, line) in lines.iter().enumerate() { let words: BTreeSet<String> = line.split_whitespace().map(|w| w.to_lowercase()).collect(); for w in words { m.entry(w).or_default().push(i + 1); } }`. Using a set per line removes in-line repeats, and lines are visited in order so the lists stay ascending.
===
`alphabetical`: `index.iter().filter(|(_, l)| l.len() > 1).map(|(w, _)| w.clone()).collect()`. `between`: `if lo > hi { return vec![]; } prices.range(lo..=hi).map(|(_, n)| n.clone()).collect()`.
===
`cheapest_at_least`: `prices.range(min..).next().map(|(_, n)| n)`. `priciest_below`: `prices.range(..limit).next_back().map(|(_, n)| n)`. `distinct_gaps`: `let s: BTreeSet<i32> = v.iter().copied().collect(); let sorted: Vec<i32> = s.into_iter().collect(); sorted.windows(2).map(|w| w[1] - w[0]).collect()`.
