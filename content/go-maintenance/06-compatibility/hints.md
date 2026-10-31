Do not edit the body of `Parse`'s loop. Move it into a shared function `parse(src, quotes bool)` and make `Parse` call it with `false` only if you are sure the legacy branch is untouched.
===
In the shared loop, branch on `quotes && strings.HasPrefix(value, "\"")` before the legacy comment stripping. Everything else falls through to the old code.
===
Quote scanning: iterate over runes after the opening quote; handle `\` by looking at the next rune. Remember to track where the closing quote is and what follows it.
===
`Parse(src)` and `ParseOpts(src, Options{})` must agree on errors too, line numbers included.
===
`Bool`: `strings.ToLower(strings.TrimSpace(v))` then a switch.
