Stdin has rule lines `RULE <name> <field> <op> <value>`, where the value is the rest of the line (it may contain spaces) and op is `eq`, `contains`, `startswith` or `endswith`. A blank line ends the rules and the remaining lines are `key=value` events (the format of the previous lesson). At most 32 rules and 200 events.

For each event number `N` (starting at 1) print `event N: ` followed by the names of the matching rules in rule order separated by commas, or `-` if none match. A missing field never matches. Tests use several rule sets and events.

Details: operators are case-sensitive; `RULE` lines always have a value; an event with no rule matching prints `event N: -`; `contains` with a single-character value works like any other; the value keeps internal spaces but not the single space after the operator. Rule names contain no spaces or commas.
