`item_total` is `it.qty * it.price_cents`, and `inventory_value` adds `item_total(items[i])` in a loop. `find_item` is a search loop: `strcmp(items[i].name, name) == 0` means the names match.
===
`restock` can call `find_item` first; if the result is `-1`, return `-1`; otherwise do `items[i].qty += amount;` and return the new `items[i].qty`. In `item_make`, copy characters with a loop that also stops at index 15: `while (name[i] != '\0' && i < 15)`.
===
```c
Item it;
int i = 0;
while (name[i] != '\0' && i < 15) {
    it.name[i] = name[i];
    i++;
}
it.name[i] = '\0';
it.qty = qty;
it.price_cents = price_cents;
return it;
```
