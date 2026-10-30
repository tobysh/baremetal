The starter defines `struct Item { name: String, qty: u32 }` and `struct Inventory { items: Vec<Item> }`. Implement these in `impl Inventory`:

- `fn new() -> Inventory` creates an empty inventory.
- `fn add(&mut self, name: &str, qty: u32)` adds `qty` to the item called `name`, creating the item if it does not exist yet. New items go on the end.
- `fn remove(&mut self, name: &str, qty: u32) -> bool` takes `qty` away from the item and returns `true`. If the item does not exist or has fewer than `qty` in stock, change nothing and return `false`.
- `fn count(&self, name: &str) -> u32` returns the quantity of `name`, or `0` if there is no such item.
- `fn total(&self) -> u32` returns the sum of all quantities.
- `fn in_stock(&self) -> Vec<String>` returns the names of items with a quantity above zero, in the order they were first added.
