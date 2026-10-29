The starter has the types, but their comparison and `Default` code is hand-written and wrong. Delete those `impl` blocks and use `#[derive(...)]` instead, then implement the functions. The types are:

- `Version { major, minor, patch }` (all `u32`): must compare, sort and hash like a version number.
- `Suit` (`Clubs`, `Diamonds`, `Hearts`, `Spades`) and `Card { rank: u8, suit: Suit }`: cards should sort by rank first, then suit, using the declared variant order.
- `Settings { name: String, retries: u32, verbose: bool, tags: Vec<String> }` with the all-zero/empty `Default`.

Then complete:

- `fn parse_version(s: &str) -> Option<Version>`: exactly three dot-separated non-negative integers like `"1.12.3"`. Anything else (`"1.2"`, `"1.2.3.4"`, `"a.b.c"`, `"1..3"`) is `None`.
- `fn newest(versions: &[Version]) -> Option<Version>`: the greatest one (returns a clone), `None` if empty.
- `fn dedup_versions(v: &[Version]) -> Vec<Version>`: sorted ascending, no duplicates.
- `fn sorted_hand(cards: &[Card]) -> Vec<Card>`: a sorted copy.
- `fn quiet_settings(name: &str) -> Settings`: defaults, except `name` set to the argument and `retries` set to `3`. Use struct update syntax.
- `fn needs_bump(a: &Version, b: &Version) -> bool`: true when `b` is a newer **major or minor** release than `a` (a patch-only difference does not count; downgrades are false).
