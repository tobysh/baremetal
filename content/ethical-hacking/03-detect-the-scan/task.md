Read stdin (at most 2000 lines, 64 sources). For every source that reaches 8 distinct ports within a 10-second window (`t - 10 < ts <= t`, counted when the line is read), print once, at the first crossing:

`SCAN <source> at=<ts> ports=<N> allowed=<K>`

`N` is the number of distinct ports from that source in the window at that moment, and `K` how many of those ports have at least one `ALLOW` event in the window. Sort rows by `at`, then by `strcmp` on the source. If nobody qualifies print `no scan detected`. Ignore malformed lines.

Different logs are used, including retry storms and slow scans.
