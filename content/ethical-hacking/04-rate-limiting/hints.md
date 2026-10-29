Find the slot for the key. For counting, only timestamps with `ts > now - window` matter, so a timestamp exactly `window` seconds old no longer counts.
===
Keep each key's timestamps in order (a ring or a shifted array). Before counting, drop expired ones, which also frees the slot when none remain. The oldest live timestamp is then `ts[0]`.
===
`retry_after = ts[0] + window - now` when the live count is `>= limit`, otherwise `0`.
