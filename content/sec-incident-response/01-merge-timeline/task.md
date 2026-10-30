Implement `BuildTimeline(srcs []Source) ([]Event, []string)`.

Each `Source` has a `Name`, a `SkewSeconds` (how many seconds that source's clock runs **ahead** of true UTC) and `Lines`. A line is an RFC 3339 timestamp (with offset, whole seconds), one space, then the message, which runs to the end of the line and may contain spaces.

Return events with `Unix` = parsed Unix time minus the source's skew, plus `Source`, `Message` and `Line` (the 1-based line number within its source). Sort by `Unix`, then `Source` name, then `Line`.

A line is rejected if the timestamp does not parse as RFC 3339 or the message is empty. Return rejected lines as `"<source>:<line>"` in input order (source order, then line order). Rejected lines produce no event. Empty input gives empty (not necessarily nil) results.

Public example: source `web` with skew 120 and line `2026-03-04T11:02:00+01:00 GET /login` gives `Unix` equal to 10:02:00Z minus 120 s, i.e. 10:00:00Z.

Hidden tests build events from known true times, then write them with random offsets and skews, shuffle them and add malformed lines.
