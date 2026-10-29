`impl Matcher for Prefix<'_> { fn matches(&self, word: &str) -> bool { word.starts_with(self.0) } }` and `MinLen` compares `word.len() >= self.0`.
===
`select`: `text.split_whitespace().filter(|w| m.matches(w)).collect()`. `first_match` is the same with `.find(...)`.
===
`select_all`: filter with `ms.iter().all(|m| m.matches(w))`. `all` on an empty list is `true`, which is exactly the rule for an empty matcher list.
