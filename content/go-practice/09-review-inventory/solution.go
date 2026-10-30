package main

import (
	"encoding/json"
	"errors"
	"fmt"
	"sort"
)

// Item is one line of stock. Prices are in cents.
type Item struct {
	SKU   string `json:"sku"`
	Qty   int    `json:"qty"`
	Price int    `json:"price,omitempty"`
}

var ErrUnknownSKU = errors.New("unknown sku")
var ErrInsufficient = errors.New("insufficient stock")

// Filter returns the elements for which keep is true.
func Filter[T any](xs []T, keep func(T) bool) []T {
	out := []T{}
	for _, x := range xs {
		if keep(x) {
			out = append(out, x)
		}
	}
	return out
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
	if cur, ok := s.items[it.SKU]; ok {
		cur.Qty += it.Qty
		s.items[it.SKU] = cur
		return
	}
	s.items[it.SKU] = it
}

// Remove takes qty units of sku out of stock.
func (s *Store) Remove(sku string, qty int) error {
	cur, ok := s.items[sku]
	if !ok {
		return fmt.Errorf("remove %s: %w", sku, ErrUnknownSKU)
	}
	if cur.Qty < qty {
		return fmt.Errorf("remove %s: have %d, want %d: %w", sku, cur.Qty, qty, ErrInsufficient)
	}
	cur.Qty -= qty
	if cur.Qty == 0 {
		delete(s.items, sku)
	} else {
		s.items[sku] = cur
	}
	return nil
}

// Sorted returns all items ordered by less, ties in SKU order.
func (s *Store) Sorted(less func(a, b Item) bool) []Item {
	items := make([]Item, 0, len(s.items))
	for _, it := range s.items {
		items = append(items, it)
	}
	sort.Slice(items, func(i, j int) bool { return items[i].SKU < items[j].SKU })
	if less != nil {
		sort.SliceStable(items, func(i, j int) bool { return less(items[i], items[j]) })
	}
	return items
}

// Where returns the items for which keep is true, in SKU order.
func (s *Store) Where(keep func(Item) bool) []Item {
	return Filter(s.Sorted(nil), keep)
}

// JSON encodes the items, in SKU order, as a JSON array.
func (s *Store) JSON() ([]byte, error) {
	return json.Marshal(s.Sorted(nil))
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
	err := l.Store.Remove(sku, qty)
	if err != nil {
		l.Log = append(l.Log, fmt.Sprintf("failed %s: %v", sku, err))
	} else {
		l.Log = append(l.Log, fmt.Sprintf("removed %d %s", qty, sku))
	}
	return err
}
