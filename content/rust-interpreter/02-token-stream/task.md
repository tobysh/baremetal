Complete `Stream::peek`, `peek_at`, `pos`, `next`, `eat`, `expect`, `expect_ident` and `at_end`.

`peek_at(0)` equals `peek`; lookahead past the end returns `None`. `pos` returns the next token's byte offset or the original source length. Failed `eat`/`expect`/`expect_ident` operations leave the cursor unchanged. `next` at EOF returns `None` without advancing.

`expect(t)` returns the consumed offset or a `ParseError` with `expected {:?}, found {:?}` using token debug formatting. At EOF the suffix is `found end of input`. `expect_ident` substitutes `identifier` for the expected token and returns its name on success.

Tests use several token sequences and repeat operations after EOF.
