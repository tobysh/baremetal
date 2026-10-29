`handle` is `Registry { inner: Arc::clone(&self.inner) }`. `get` is `self.inner.read().unwrap().get(key).copied()`.
===
`incr` must be a single write-lock section: `let mut m = self.inner.write().unwrap(); let v = m.entry(key.to_string()).or_insert(0); *v += 1; *v`.
===
`snapshot`: read lock, `.iter().map(|(k, v)| (k.clone(), *v)).collect()`, then `sort()`. `can_read_now` is `lock.try_read().is_ok()`.
