The `Limiter` structure is given. Implement:

- `void rl_init(Limiter *l, int limit, long window)` empties the table and stores the settings (`limit` is 1 to 16, `window` is at least 1).
- `int rl_allow(Limiter *l, const char *key, long now)` returns `1` and records the request when fewer than `limit` earlier allowed requests of this key have `ts > now - window`; otherwise returns `0` and records nothing. A key not yet in the table needs a free slot: reuse a slot whose requests are all expired; if every slot holds a live key, return `0`.
- `long rl_retry_after(Limiter *l, const char *key, long now)` returns `0` if a request would be allowed now (or the key is unknown), otherwise the number of seconds until it would be: `oldest_live_ts + window - now`.

Keys are at most 31 characters. Time never goes backwards. Many random request streams are replayed.
