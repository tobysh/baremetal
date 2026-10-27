Complete these in the starter. `PortError` is `#[derive(Debug, PartialEq)]` with variants `Missing`, `Empty`, `NotANumber(ParseIntError)` and `OutOfRange(i64)`.

- `impl Display for PortError`, with these exact messages:
  - `Missing` gives `no port configured`
  - `Empty` gives `port is empty`
  - `NotANumber(e)` gives `not a number: ` followed by the inner error's own message
  - `OutOfRange(n)` gives `port 70000 is out of range (1-65535)`, with the number in place
- `impl From<ParseIntError> for PortError` (a placeholder is in the starter), producing `NotANumber`.
- `fn parse_port(s: &str) -> Result<u16, PortError>`: trim whitespace; an empty result is `Empty`; parse as `i64` using `?`; anything outside `1..=65535` (including negatives) is `OutOfRange(n)`.
- `fn lookup<'a>(config: &'a str, key: &str) -> Option<&'a str>`: `config` has one `key=value` pair per line. Return the value (trimmed) of the first line whose key (trimmed) equals `key`. Lines without `=` are ignored.
- `fn port_from_config(config: &str) -> Result<u16, PortError>`: look up the key `port`; a missing key is `Missing`; otherwise parse it with `parse_port`.
- `fn halve_even(n: Option<i32>) -> Option<i32>`: `Some(n / 2)` when `n` is an even number, otherwise `None`. Use `filter` and `map`.
