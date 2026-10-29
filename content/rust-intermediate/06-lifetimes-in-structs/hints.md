`next`: `let s = self.rest.trim_start();` and if it is empty return `None`. Otherwise `let end = s.find(char::is_whitespace).unwrap_or(s.len());`, then `let (tok, rest) = s.split_at(end); self.rest = rest; Some(tok)`.
===
`parse_record`: `let (k, v) = line.split_once('=')?;`, trim both, return `None` if the key is empty. `parse_all` is `text.lines().filter_map(parse_record).collect()`.
===
`longest_value`: loop over the records, keep `let mut best: Option<&'a str> = None;`, and replace it when the value is strictly longer than the current best (treat `None` as always replaceable). Return a clone of the `&'a str` field, not a reference to the record.
