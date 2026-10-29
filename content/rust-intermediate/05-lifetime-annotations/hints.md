`longest`: `if b.len() > a.len() { b } else { a }`. `first_word` can use `s.split_whitespace().next().unwrap_or("")`.
===
`split_pair`: `let i = s.find(sep)?;` gives the byte index of the separator. The part before is `&s[..i]` and the part after starts at `i + sep.len_utf8()`.
===
`strip_known` is `text.strip_prefix(prefix).unwrap_or(text)`. For `longest_line` keep `let mut best = "";` and replace it with a line only when `line.len() > best.len()`.
