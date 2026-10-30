Tags: `` SKU string `json:"sku"` ``, `` Qty int `json:"qty"` ``, `` Price int `json:"price,omitempty"` ``. The sentinels are `errors.New("unknown sku")` and `errors.New("insufficient stock")`.
===
`Filter`: start with `out := []T{}` so the result is never nil, then loop and append.
===
`Store` keeps `map[string]Item`. `Add`: look up with `cur, ok := s.items[it.SKU]`; if found, `cur.Qty += it.Qty` and store it back (map values are copies). `Remove`: check `ok` first, then `cur.Qty < qty`, then subtract and `delete` at zero. Wrap with `fmt.Errorf("remove %s: %w", sku, ErrUnknownSKU)`.
===
`Sorted`: collect the map values into a slice, `sort.Slice` it by SKU first, then `sort.SliceStable` with `less(items[i], items[j])` if `less != nil`. Stability keeps the SKU order for ties. `Where` is `Filter(s.Sorted(nil), keep)` and `JSON` is `json.Marshal(s.Sorted(nil))`.
===
`LoggedStore.Remove`: `err := l.Store.Remove(sku, qty)`, then `if err != nil { l.Log = append(l.Log, fmt.Sprintf("failed %s: %v", sku, err)) } else { ... }`, and `return err`. Use the pointer receiver `*LoggedStore`; `NewLoggedStore` returns `&LoggedStore{Store: *NewStore()}`.
