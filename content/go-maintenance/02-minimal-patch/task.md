Fix the apostrophe bug in `Title`: an apostrophe (`'`) or a typographic apostrophe (`’`) inside a word must not make the next letter a capital. `Title("don't stop")` is `Don't Stop` and `Title("rock'n'roll")` is `Rock'n'roll`.

Everything else must behave exactly as before, including these documented quirks:

- Words are split on whitespace and joined with one space.
- Small words (`a an and of the in to`) are lower-case unless they are the first word.
- A word of two or more letters that is already all upper case (`NASA`) is kept.
- Any other non-letter, such as `-`, `/`, `(` or `.`, makes the next letter a capital, so `well-known` is `Well-Known` and `and/or` is `And/Or`.
- Digits also count as non-letters, so `3rd` is still `3Rd`. That is a known quirk; **do not fix it here**.
- `NameCase` capitalises after an apostrophe on purpose (`o'brien` is `O'Brien`). It is not part of this bug.

Change only what the report needs. The other functions (`Heading`, `Slug`, `Truncate`) are covered by tests too.
