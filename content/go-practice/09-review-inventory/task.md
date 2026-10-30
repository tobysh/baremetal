Complete the inventory:

- Give `Item` JSON tags: `sku`, `qty`, and `price` with `omitempty`.
- `ErrUnknownSKU` and `ErrInsufficient` are sentinel errors (`errors.New`).
- `Filter[T any](xs []T, keep func(T) bool) []T` returns the elements for which `keep` is true, in order. It never returns `nil` (an empty result is an empty slice).
- `NewStore() *Store`, and `(s *Store) Add(it Item)`: a new SKU is stored; an existing SKU has `it.Qty` added to its quantity (its price is left alone).
- `(s *Store) Remove(sku string, qty int) error` takes stock away. An unknown SKU returns an error that wraps `ErrUnknownSKU`; asking for more than is in stock returns an error that wraps `ErrInsufficient`. Both messages start with `"remove SKU: "`. When the quantity reaches exactly 0, the SKU is deleted from the store.
- `(s *Store) Sorted(less func(a, b Item) bool) []Item` returns all items ordered by `less`. Items that `less` treats as equal come out in SKU order. A `nil` `less` means plain SKU order.
- `(s *Store) Where(keep func(Item) bool) []Item` returns the items for which `keep` is true, in SKU order, using your `Filter`.
- `(s *Store) JSON() ([]byte, error)` encodes the items in SKU order as a JSON array.
- `LoggedStore` embeds `Store` and has a `Log []string`. `NewLoggedStore()` makes one. Its `Remove` calls the embedded `Remove` and appends `"removed QTY SKU"` on success or `"failed SKU: ERR"` (the error's text) on failure, and returns the same error.
