Evaluate in two steps for each event: first compute true or false for every named selection (into a small table indexed by the letter), then evaluate the condition string against that table.
===
Tokenise the condition once. Then write three functions that mirror precedence: `parse_or` calls `parse_and`, which calls `parse_not`, which handles `not`, a parenthesised group (calling `parse_or` again) or a selection name. Each loop level keeps going while the next token is its own operator.
===
If `a or b and c` gives the wrong answer, `or` and `and` are at the wrong level. If a later part of the expression is ignored after a false `and`, you skipped parsing the right side instead of parsing it and combining afterwards. If nested groups break, check that you consume the `)`.
===
The same shape on a different grammar, sums and products:
```c
static int term(void)  { int v = factor(); while (tok_is("*")) { next(); v *= factor(); } return v; }
static int expr(void)  { int v = term();   while (tok_is("+")) { next(); v += term();   } return v; }
```
