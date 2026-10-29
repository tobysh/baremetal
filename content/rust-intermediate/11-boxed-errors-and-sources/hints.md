`read_pair`: `let (a, b) = s.split_once(',').ok_or("expected 'a,b'")?;` then `Ok((a.trim().parse()?, b.trim().parse()?))`. The `?` converts the `&str` and `ParseIntError` into `Box<dyn Error>` for you.
===
`source()` returns `Some(&self.source)`. `load` is `text.trim().parse().map_err(|e| LoadError { name: name.to_string(), source: e })`. `load_all` is a loop with `out.push(load(name, text)?)`; `?` boxes the `LoadError`.
===
`chain`: `let mut out = vec![e.to_string()]; let mut cur = e.source(); while let Some(s) = cur { out.push(s.to_string()); cur = s.source(); }`. `is_int_error` is `e.downcast_ref::<ParseIntError>().is_some()`.
