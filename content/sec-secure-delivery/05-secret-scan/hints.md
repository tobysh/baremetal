Compile the two regular expressions once at package level and, for each line, use `FindAllStringSubmatchIndex`. For the assignment pattern, group 2 gives the value's start and end.
===
Strip one trailing `\r` from each line before matching. Column is `start+1`.
===
Collect candidates for a line as `{col, rule, secret}`, apply the placeholder and length rules to assignments, drop an assignment whose value starts at the same column as an `svc-token`, then sort by column.
===
Apply the baseline last: `if baseline[Fingerprint(secret)] { continue }`.
