First line: `W N`. Then at most 2000 lines `<timestamp> <key>` in non-decreasing time order (keys at most 63 characters, at most 64 distinct keys). When an event arrives, count events with the same key and timestamp `> t - W` (including this one). If the count is at least `N` and the key is not suppressed, print `<t> <key> count=<count>` and suppress that key until timestamp `t + W` (an event at exactly `t + W` may alert again). If nothing alerts print `no alerts`.

Tests use several window sizes, including events exactly `W` apart and several events in the same second.

Details: timestamps are integers and never decrease; `W` and `N` are at least 1; the key is one token with no spaces; with `N=1` every event not suppressed alerts; events of other keys never affect a key's count or suppression; alerts are printed in input order, one per line, exactly as `<t> <key> count=<count>`.
