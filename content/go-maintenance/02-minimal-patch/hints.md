Find where `Title` decides that the next letter should be a capital. The state variable is called `up`.
===
`titleWord` and `nameWord` contain the same line: `up = !unicode.IsLetter(r)`. Only the one used by `Title` is wrong; `NameCase` relies on its behaviour.
===
In `titleWord`, treat `'` and `’` as "do not change the state": keep the previous value of `up` (which will already be false after a letter). Do not remove the rule for other non-letters.
===
Check the quirks after editing: `3rd` must still give `3Rd`, and `o'brien` through `NameCase` must still give `O'Brien`. Run the example in `main`.
