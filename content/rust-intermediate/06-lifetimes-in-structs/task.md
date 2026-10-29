Work with text without copying it.

- `Tokens::next`: yield the next whitespace-separated word of the remaining text, as a `&'a str`. Do not use `split_whitespace`; keep the unread remainder in `rest`. Tabs and newlines are whitespace.
- `parse_record(line)`: a line of the form `key = value` becomes `Some(Record { key, value })` with both parts trimmed. Split at the first `=`. A line with no `=`, or with an empty key, is `None`. The value may be empty.
- `parse_all(text)`: the records of all valid lines, in order; bad lines are skipped.
- `longest_value(records)`: the longest value by byte length (first on a tie), or `None` for no records. The result must stay usable after the `records` slice is dropped.
