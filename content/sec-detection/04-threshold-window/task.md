First line: `W N`. Then at most 2000 lines `<timestamp> <key>` in non-decreasing time order (keys at most 63 characters, at most 64 distinct keys). When an event arrives, count events with the same key and timestamp `> t - W` (including this one). If the count is at least `N` and the key is not suppressed, print `<t> <key> count=<count>` and suppress that key until timestamp `t + W` (an event at exactly `t + W` may alert again). If nothing alerts print `no alerts`.

Tests use several window sizes, including events exactly `W` apart and several events in the same second.
