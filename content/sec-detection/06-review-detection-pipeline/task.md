Stdin: `W N`, then zero or more `ALLOW <src-glob>` lines, a blank line, then events `<timestamp> <fail|ok> <user> <src>` in time order (at most 1000). Count `fail` events per source in the window `(t-W, t]`; at count `N` or more, and unless suppressed (see below) and not allowlisted, print `ALERT <t> <src> failures=<count> first-ok=<ts|none>`. After alerting, suppress that source until `t + W`. `*` matches any run and `?` one character in globs. Print `no alerts` if nothing fires. Print alerts in time order.

Tests use several parameter sets and sources.
