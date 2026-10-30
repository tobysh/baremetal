`middle`: check `v.len() <= 2` first and return `&[]`. Otherwise `&v[1..v.len() - 1]`.
===
`last_word`: `s.split_whitespace().last().unwrap_or("")`. For `split_at_char`, `match s.find(c)` gives `Some(i)`, a byte position; the text after the character starts at `i + c.len_utf8()`.
===
`split_at_char`: `Some(i) => (&s[..i], &s[i + c.len_utf8()..]), None => (s, "")`. For `join_dash`, slices of strings have a `join` method: `parts.join("-")`.
