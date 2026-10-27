`longest`: compare lengths with `a.len() >= b.len()` and return the matching reference. The lifetime annotations are already in the signature.
===
`first_word`: `match s.find(' ') { Some(i) => &s[..i], None => s }`.
===
`next_word`: first `let trimmed = self.rest.trim_start();`. If `trimmed` is empty, set `self.rest = trimmed` and return `None`.
===
Otherwise find the end: `let end = trimmed.find(' ').unwrap_or(trimmed.len());`. Then `let (word, rest) = trimmed.split_at(end);`, set `self.rest = rest;` and return `Some(word)`. Both slices borrow from the original text, so the `'a` return type is satisfied.
