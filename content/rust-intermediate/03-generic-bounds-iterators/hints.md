`Countdown::next`: if `self.n == 0` return `None`; otherwise save `let v = self.n;`, decrement, and return `Some(v)`.
===
For `largest_by_key`, keep `best: Option<&T>`. For each item, replace `best` only if there is none or `key(item) > key(best)` (strictly, to keep the first on a tie).
===
`summarize`: `it.into_iter().map(|x| x.to_string()).collect::<Vec<_>>().join(", ")`. `every_other` is `it.step_by(2).collect()`.
