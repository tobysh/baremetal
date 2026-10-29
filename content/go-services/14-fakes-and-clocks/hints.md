`SystemClock`: `type SystemClock struct{}` with `func (SystemClock) Now() time.Time { return time.Now() }`. `FakeClock` is in the lesson; add a constructor `return &FakeClock{now: start}`.
===
`TTLCache` fields: `clock Clock`, `ttl time.Duration`, `mu sync.Mutex`, `m map[string]entry` where `entry` has `value string` and `expires time.Time`.
===
`Set`: `c.m[key] = entry{value, c.clock.Now().Add(c.ttl)}` under the lock. A second `Set` replaces the entry, which restarts its TTL.
===
`Get`: look the entry up; it is expired when `!c.clock.Now().Before(e.expires)`. In that case `delete(c.m, key)` and return `"", false`. `Len`: loop over the map, deleting the expired entries, then return `len(c.m)`.
