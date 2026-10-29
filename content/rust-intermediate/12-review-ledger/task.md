Complete `Display`, `source` and `apply` for the ledger.

`fn apply(text: &str) -> Result<BTreeMap<String, i64>, LedgerError>` replays the ledger. Lines are trimmed; blank lines are skipped; line numbers start at 1 and count blank lines. A line is `name:amount`, split at the first `:`, with both parts trimmed.

- A line with no `:`, or an empty name, is `Syntax(line)`.
- An amount that does not parse as an `i64` (a leading `+` or `-` is allowed) is `Amount { line, source }`.
- If applying the amount would make that account's balance negative, the error is `Overdraft { name, balance }` where `balance` is the negative balance it would have reached.
- The first error is returned. Accounts start at 0, and every account that appears in the ledger is in the result, even at balance 0.

`Display` messages:

| Variant | Message |
| --- | --- |
| `Syntax(3)` | `line 3: expected name:amount` |
| `Amount { line: 2, .. }` | `line 2: bad amount (<the parse error's message>)` |
| `Overdraft { name: "bob", balance: -5 }` | `bob would go to -5` |

`source()` is the `ParseIntError` for `Amount` and `None` otherwise.

`fn report(text: &str) -> String`: `name=balance` pairs joined with `, ` in name order on success, or `error: ` followed by the error message.
