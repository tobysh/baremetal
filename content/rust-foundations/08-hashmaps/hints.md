Create a `let mut counts = HashMap::new();`, loop over `text.split_whitespace()`, clean each word, and return `counts` at the end.
===
Clean a word with `let w = raw.to_lowercase();` then `let w = w.trim_matches(|c: char| c.is_ascii_punctuation());`. If `w.is_empty()`, use `continue` to skip it.
===
Count with the entry API: `*counts.entry(w.to_string()).or_insert(0) += 1;`. The keys must be owned `String`s, so convert the `&str` with `to_string()`.
