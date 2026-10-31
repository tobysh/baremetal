Legacy rules (do not change `Parse`, `Get`, `Has`, `Keys` or `Int`):

- Lines are split on `\n`, trimmed; blank lines and lines starting with `#` or `;` are skipped.
- A line without `=` is an error `line N: missing '='`. An empty key is `line N: empty key`.
- Key = trimmed text before the first `=`; value = trimmed text after it. A `#` that is preceded by a space (or starts the value) begins a comment which is removed; `a#b` keeps its `#`. Quotes are not special.
- A repeated key: the last value wins, the key keeps its first position. `Keys()` is in first-seen order. `Int(key, def)` uses `strconv.Atoi` on the value and returns `def` on any error.

Add:

1. `type Options struct { Quotes bool }` and `ParseOpts(src string, o Options) (*Config, error)`. With the zero `Options`, the result is identical to `Parse` for every input, including errors.
2. With `Quotes: true`, a value that begins with `"` is read up to the next unescaped `"`. Escapes inside: `\"`, `\\`, `\n` (newline); any other escape is the error `line N: bad escape`. The text kept is exactly what is inside the quotes (no trimming, ` #` is plain text). After the closing quote only spaces or a ` #` comment may follow, otherwise `line N: junk after quoted value`. A missing closing quote is `line N: unterminated quote`. Values that do not start with `"` behave as in legacy.
3. `(*Config).Bool(key string, def bool) bool`: values `true yes on 1` and `false no off 0` in any letter case; anything else (or missing) gives `def`.

`Parse` itself stays legacy: it must never interpret quotes.
