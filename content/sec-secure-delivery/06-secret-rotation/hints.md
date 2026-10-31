Store each version as `{secret string, end int64, revoked bool}` and keep the time of the last rotation. The current version has `end = math.MaxInt64`.
===
In `Rotate`: validate everything first (length, reuse across all versions, clock), and only then mutate. Shorten the previous version with `end = min(end, now+grace)`, using a saturating add if `now+grace` could overflow.
===
In `Check`: loop over all versions without returning early, record the highest matching version that is `!revoked && now < end`, and return after the loop.
