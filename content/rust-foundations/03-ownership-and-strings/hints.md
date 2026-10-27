For `reverse`, a `String` is built from characters, not bytes. `s.chars()` gives an iterator of characters, and iterators have a `.rev()` method.
===
Chain it: `s.chars().rev().collect()`. The return type `String` tells `collect` what to build. For `first_word`, `s.find(' ')` returns an `Option<usize>`, the byte position of the first space.
===
Use `match s.find(' ') { Some(i) => &s[..i], None => s }`. Slicing at a space position is safe because a space is a single byte.
