Stdin: lines `SEL <name> <field> <op> <value>` (ops as in the previous lesson, value to end of line, names are lowercase letters), then one line `COND <expression>`, then a blank line, then `key=value` events. Tokens in the expression are separated by single spaces: names, `and`, `or`, `not`, `(` and `)`. Precedence is `not` > `and` > `or`. At most 16 selections and 200 events.

For each event print `event N: match` or `event N: no match`. Tests use many expressions and events, including nested parentheses.

Details: selection names are single lowercase letters and every name used in the condition is defined; the condition is always well formed (balanced parentheses, one space between tokens); `not not a` is allowed; a selection on a missing field is false; the event lines use the key=value format with first-occurrence semantics. Selection values run to the end of the line and may contain spaces.
