`first_duplicate`: `let mut seen = HashSet::new(); for &x in v { if !seen.insert(x) { return Some(x); } } None`.
===
`word_counts`: `for w in text.split_whitespace() { *m.entry(w.to_lowercase()).or_insert(0) += 1; }`. Grouping: `let c = w.chars().next()?` does not work inside a loop in a non-Option function, so use `if let Some(c) = w.chars().next() { m.entry(c.to_lowercase().next().unwrap()).or_default().push(w.to_string()); }`.
===
Sets: `let sb: HashSet<i32> = b.iter().copied().collect(); let mut out: Vec<i32> = a.iter().copied().collect::<HashSet<_>>().into_iter().filter(|x| sb.contains(x)).collect(); out.sort();`. `only_in_first_sorted` is the same with `!sb.contains(x)`. `most_common`: sort the map's entries by `(Reverse(count), word)` and take the first, or use `min_by_key(|(w, c)| (Reverse(**c), (*w).clone()))`.
