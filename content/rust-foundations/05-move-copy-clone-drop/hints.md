`Drop` looks like this: `impl Drop for Guard { fn drop(&mut self) { ... } }`. Inside, build the text with `format!("drop {}", self.name)` and push it with `self.log.borrow_mut().push(text);`.
===
`clone_all`: `items.to_vec()` or `items.iter().cloned().collect()`. Both clone each `String`. For `with_added`, declare the parameter `mut v` in the signature (it already is), then `v.push(extra); v`.
===
`run_scope`: `let a = Guard::new(first, log); let b = Guard::new(second, log); drop(a); log.borrow_mut().push("work".to_string());`. Do not drop `b` yourself; it is dropped when the function ends. Keep `b` in a named variable (not `_`), or it would be dropped immediately.
