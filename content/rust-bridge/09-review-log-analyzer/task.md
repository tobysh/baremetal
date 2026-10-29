The starter defines `struct Entry { level: String, module: String, message: String }`. Complete:

- `fn parse_line(line: &str) -> Option<Entry>`: trim the line, take the first word as the level (store it **uppercase**), the second word must end with `:` and becomes the module (without the colon, as written), and the rest, trimmed, is the message. `None` if there is no level, no colon-terminated module, or the module is empty after removing the colon. A message may be empty.
- `fn parse_all(text: &str) -> (Vec<Entry>, Vec<usize>)`: parse every non-blank line of `text`. Return the entries and the 1-based numbers of the lines that were not blank but failed to parse. Line numbers count blank lines too.
- `fn count_levels(entries: &[Entry]) -> BTreeMap<String, usize>`: how many entries per level.
- `fn busiest_module(entries: &[Entry]) -> Option<(String, usize)>`: the module with the most entries (ties: alphabetically first). `None` if there are no entries.
- `fn messages_for(entries: &[Entry], level: &str) -> Vec<String>`: the non-empty messages of entries with that level (compare case-insensitively), in order.
