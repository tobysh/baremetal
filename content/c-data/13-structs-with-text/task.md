`Item` is defined in the starter; `name` holds up to 15 characters.

- `Item item_make(char name[], int qty, int price_cents)` returns an item with those values. A name longer than 15 characters is cut to its first 15. The `name` field must always end in a terminator. Write the copying loop yourself (no `strcpy` or `strncpy`).
- `int item_total(Item it)` returns `qty * price_cents`.
- `int inventory_value(Item items[], int n)` returns the sum of `item_total` over the table.
- `int find_item(Item items[], int n, char name[])` returns the index of the first item whose name equals `name`, or `-1`.
- `int restock(Item items[], int n, char name[], int amount)` adds `amount` to the quantity of the named item and returns the new quantity, or returns `-1` (changing nothing) if no item has that name.
