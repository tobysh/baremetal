Start with parsing only: write `parseV2(rest string)` that splits on unescaped `|`. Escaped pipes make `strings.Split` insufficient; scan rune by rune and build fields.
===
In `parseLine`, dispatch: if the line starts with `v2|` use the v2 parser; else if it looks like `v<digits>|` report the unsupported version; else use the existing v1 code unchanged.
===
`Save` just formats every member as v2. Write `escape` and `unescape` and test `unescape(escape(x)) == x` for strings containing `|`, `\`, and newlines.
===
`Migrate`: split the data on `\n`, trim a trailing `\r` for classification only, copy blank/comment lines as they are (without `\r`), and rewrite record lines with the v2 formatter. Return early with `""` on the first error.
===
Idempotence follows if v2 lines are re-formatted by the same canonical function: parse then format.
