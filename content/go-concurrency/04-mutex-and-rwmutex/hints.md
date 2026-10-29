Add a `mu sync.Mutex` field to `Counter`. In `Inc`: `c.mu.Lock()`, `defer c.mu.Unlock()`, `c.n++`. `Value` needs the lock too, because a read racing a write is still a race.
===
`Registry` holds `mu sync.RWMutex` and `m map[string]int`. `NewRegistry` must `make` the map.
===
`Get` and `Len` use `RLock`/`defer RUnlock`. `Set` and `Incr` use `Lock`/`defer Unlock`.
===
`Incr` must read and write inside the same `Lock` section: `r.m[key]++; return r.m[key]`.
