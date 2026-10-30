`time.Parse(time.RFC3339, ...)` handles the offset for you; use `.Unix()` afterwards.
===
Split with `strings.Cut(line, " ")`. A line with no space, or nothing after it, has no message.
===
Use `sort.SliceStable` or a comparison that checks Unix, then Source, then Line, so ties never depend on input order.
