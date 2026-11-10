Read lines with `fgets`. For each one, `sscanf(line, "%lld %63s %n", &t, src, &at)` finds the time and source, and `at` is then the index where the message starts. Strip the trailing newline from the message.
===
Store the events in an array of structs and sort them yourself with insertion sort, shifting only elements whose time is strictly greater than the one being inserted. That keeps equal times in input order.
===
Format times with `gmtime` and `strftime("%Y-%m-%dT%H:%M:%SZ", ...)`. Use `%lld` and `long long` for the gap arithmetic, with `gap / 3600` hours and `gap % 3600 / 60` minutes printed as `%02lld`.
===
Print the quiet line before the event (never before the first one) and only when the gap is at least 3600. The summary line uses the same hours and minutes formatting on `last - first`, and a single event gives `1 events over 0h 00m`.
