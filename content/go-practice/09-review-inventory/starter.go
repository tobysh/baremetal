package main

// Item is one line of stock. Prices are in cents.
type Item struct {
	SKU   string
	Qty   int
	Price int
}

var ErrUnknownSKU error
var ErrInsufficient error

// Filter returns the elements for which keep is true.
func Filter[T any](xs []T, keep func(T) bool) []T {
	return nil
}

// Store holds items by SKU.
type Store struct {
	items map[string]Item
}

func NewStore() *Store {
	return &Store{items: map[string]Item{}}
}

// Add stores an item, or adds to the quantity of an existing SKU.
func (s *Store) Add(it Item) {
}

// Remove takes qty units of sku out of stock.
func (s *Store) Remove(sku string, qty int) error {
	return nil
}

// Sorted returns all items ordered by less, ties in SKU order.
func (s *Store) Sorted(less func(a, b Item) bool) []Item {
	return nil
}

// Where returns the items for which keep is true, in SKU order.
func (s *Store) Where(keep func(Item) bool) []Item {
	return nil
}

// JSON encodes the items, in SKU order, as a JSON array.
func (s *Store) JSON() ([]byte, error) {
	return nil, nil
}

// LoggedStore is a Store that remembers what happened to removals.
type LoggedStore struct {
	Store
	Log []string
}

func NewLoggedStore() *LoggedStore {
	return &LoggedStore{Store: *NewStore()}
}

// Remove removes stock and records the outcome in Log.
func (l *LoggedStore) Remove(sku string, qty int) error {
	return nil
}
