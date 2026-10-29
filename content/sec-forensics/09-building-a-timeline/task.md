Read stdin (1..1000 lines, each at most 511 characters) in the format `UnixSeconds source message`. Sources have no spaces; seconds are nonnegative and fit time_t. Sort stably by timestamp. Print `YYYY-MM-DDTHH:MM:SSZ [source] message`. Before gaps >=3600s print `-- quiet for Hh MMm --`. Finish `N events over Hh MMm`. Minutes are zero-padded, hours are not. No blank lines. The supplied dataset always has valid lines.

Tests use several inputs, including boundary values. Keep the supplied function names and types.
