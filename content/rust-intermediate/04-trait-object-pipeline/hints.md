Each stage's `apply` is small: `x.checked_add(self.0)`, `x.checked_mul(self.0)`, `if x >= 0 { Some(x) } else { None }`, and `Some(x.min(self.0))`.
===
For `build`, split with `spec.split_once(':')`, which returns `Option<(&str, &str)>`. Then `match (kind, arg)` with arms like `("add", Some(a)) => Some(Box::new(AddN(a.parse().ok()?)))` and `("nonneg", None) => ...`.
===
`build_all` is `specs.iter().map(|s| build(s)).collect()`. `run` loops with `v = s.apply(v)?;`. `names` maps `s.name()`.
