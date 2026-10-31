Current behaviour (keep it):

- `Load`: lines split on `\n`, trailing `\r` removed, whitespace trimmed, blank and `#` lines skipped. Fields split on `,`, each trimmed. Errors are `line N: ...` (`want 2 or 3 fields`, `bad quantity`, `bad price`, `duplicate sku`, empty sku).
- `Save`: items sorted by sku, `sku,qty,price` per line, trailing newline.
- `Adjust(sku, delta)`: delta 0 is a no-op; an unknown sku with positive delta creates it with price 0; an unknown sku with negative delta is an error; quantity may not go below 0.
- `SetPrice`, `Qty`, `Value` (sum of qty*price over physical stock), `Low(threshold)` (skus with physical qty below the threshold, ordered by qty then sku).

Add:

1. `Reserve(sku, n)`: `n > 0`, sku exists, and at least `n` available (`qty - reserved`), else error. `Release(sku, n)` returns `n` from reserved (`n > 0`, at most the reserved amount). `Fulfil(sku, n)` ships reserved stock: `n > 0`, at most the reserved amount; both `qty` and `reserved` drop by `n`. `Available(sku)` is `qty - reserved` (0 for unknown).
2. `Adjust` must not take quantity below the reserved amount (error `insufficient`).
3. `Load` also reads 4-field lines `sku,qty,price,reserved`; the reserved value must be between 0 and qty (`line N: bad reserved`). Two-field lines still mean price 0.
4. `Save` writes a 4th field **only** for items with `reserved > 0`; every other item is a 3-field line, byte-identical to before.
5. `Upgrade(data string) (string, error)` converts every record line into canonical form (2 fields get price 0; a 4-field line with reserved 0 becomes 3 fields), keeps blank and comment lines unchanged (without any `\r`) and record order, and joins with `\n` (a missing final newline stays missing). It must be idempotent. Errors use the same text as `Load` (including duplicate skus) and return `""`.

`Low` and `Value` keep using **physical** quantity.
