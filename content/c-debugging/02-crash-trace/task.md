`price_of(list, name)` returns the price in cents of the item called `name`, and `order_total(list, names, n)` sums the prices of the `n` names.

- `price_of` returns `-1` if no item has that name, also when `list` is `NULL`.
- `order_total` adds only the names that exist, ignores unknown ones, and returns `0` for `n == 0` or an empty list.
- A name that appears twice is counted twice.
- Neither function may crash, whatever the names are.

Several lists and orders are tried, including a missing name first, last and in the middle, and an empty list.
