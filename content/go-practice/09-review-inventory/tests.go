package main

import (
	"errors"
	"reflect"
	"strings"
	"testing"
)

func fill(s *Store) {
	s.Add(Item{"pear", 5, 120})
	s.Add(Item{"apple", 10, 90})
	s.Add(Item{"fig", 5, 300})
	s.Add(Item{"plum", 0, 0})
}

func skus(items []Item) []string {
	out := []string{}
	for _, it := range items {
		out = append(out, it.SKU)
	}
	return out
}

func TestFilter_generic_over_types(t *testing.T) {
	got := Filter([]int{1, 2, 3, 4, 5, 6}, func(n int) bool { return n%2 == 0 })
	if !reflect.DeepEqual(got, []int{2, 4, 6}) {
		t.Errorf("Filter evens = %v; want [2 4 6]", got)
	}
	words := Filter([]string{"go", "c", "rust", "zig"}, func(s string) bool { return len(s) > 2 })
	if !reflect.DeepEqual(words, []string{"rust", "zig"}) {
		t.Errorf("Filter long words = %v; want [rust zig]", words)
	}
	none := Filter([]int{1, 3}, func(n int) bool { return n > 10 })
	if none == nil || len(none) != 0 {
		t.Errorf("Filter with no matches = %#v; want an empty non-nil slice", none)
	}
}

func TestAdd_merges_quantities_and_keeps_price(t *testing.T) {
	s := NewStore()
	s.Add(Item{"fig", 5, 300})
	s.Add(Item{"fig", 3, 999})
	got := s.Sorted(nil)
	if !reflect.DeepEqual(got, []Item{{"fig", 8, 300}}) {
		t.Errorf("after adding fig twice: %v; want [{fig 8 300}]", got)
	}
}

func TestRemove_success_and_exact_zero(t *testing.T) {
	s := NewStore()
	fill(s)
	if err := s.Remove("apple", 4); err != nil {
		t.Fatalf("Remove(apple, 4) = %v", err)
	}
	if got := s.Where(func(it Item) bool { return it.SKU == "apple" }); len(got) != 1 || got[0].Qty != 6 {
		t.Errorf("apple after removing 4 of 10: %v; want qty 6", got)
	}
	if err := s.Remove("apple", 6); err != nil {
		t.Fatalf("Remove(apple, 6) = %v", err)
	}
	if got := skus(s.Sorted(nil)); !reflect.DeepEqual(got, []string{"fig", "pear", "plum"}) {
		t.Errorf("after removing all apples the SKUs are %v; want [fig pear plum]", got)
	}
}

func TestRemove_errors_wrap_sentinels(t *testing.T) {
	if ErrUnknownSKU == nil || ErrInsufficient == nil {
		t.Fatalf("create ErrUnknownSKU and ErrInsufficient with errors.New")
	}
	s := NewStore()
	fill(s)
	err := s.Remove("kiwi", 1)
	if !errors.Is(err, ErrUnknownSKU) || errors.Is(err, ErrInsufficient) {
		t.Errorf("Remove(kiwi) = %v; want an error wrapping ErrUnknownSKU only", err)
	}
	if err != nil && !strings.HasPrefix(err.Error(), "remove kiwi: ") {
		t.Errorf("error text %q should start with \"remove kiwi: \"", err)
	}
	err = s.Remove("fig", 6)
	if !errors.Is(err, ErrInsufficient) || errors.Is(err, ErrUnknownSKU) {
		t.Errorf("Remove(fig, 6) with 5 in stock = %v; want an error wrapping ErrInsufficient only", err)
	}
	if err != nil && !strings.HasPrefix(err.Error(), "remove fig: ") {
		t.Errorf("error text %q should start with \"remove fig: \"", err)
	}
	if got := s.Where(func(it Item) bool { return it.SKU == "fig" }); len(got) != 1 || got[0].Qty != 5 {
		t.Errorf("a failed Remove changed the stock: %v", got)
	}
}

func TestSorted_default_and_custom_order(t *testing.T) {
	s := NewStore()
	fill(s)
	if got := skus(s.Sorted(nil)); !reflect.DeepEqual(got, []string{"apple", "fig", "pear", "plum"}) {
		t.Errorf("Sorted(nil) = %v; want SKU order", got)
	}
	byQtyDesc := func(a, b Item) bool { return a.Qty > b.Qty }
	if got := skus(s.Sorted(byQtyDesc)); !reflect.DeepEqual(got, []string{"apple", "fig", "pear", "plum"}) {
		t.Errorf("Sorted(qty descending) = %v; want [apple fig pear plum] (fig and pear tie, so SKU order)", got)
	}
	byPrice := func(a, b Item) bool { return a.Price < b.Price }
	if got := skus(s.Sorted(byPrice)); !reflect.DeepEqual(got, []string{"plum", "apple", "pear", "fig"}) {
		t.Errorf("Sorted(price ascending) = %v; want [plum apple pear fig]", got)
	}
	byQtyAsc := func(a, b Item) bool { return a.Qty < b.Qty }
	if got := skus(s.Sorted(byQtyAsc)); !reflect.DeepEqual(got, []string{"plum", "fig", "pear", "apple"}) {
		t.Errorf("Sorted(qty ascending) = %v; want [plum fig pear apple]", got)
	}
}

func TestWhere_filters_in_sku_order(t *testing.T) {
	s := NewStore()
	fill(s)
	got := s.Where(func(it Item) bool { return it.Qty >= 5 })
	if !reflect.DeepEqual(skus(got), []string{"apple", "fig", "pear"}) {
		t.Errorf("Where(qty >= 5) = %v; want [apple fig pear]", skus(got))
	}
	if none := s.Where(func(Item) bool { return false }); none == nil || len(none) != 0 {
		t.Errorf("Where with no matches = %#v; want empty non-nil", none)
	}
}

func TestJSON_uses_tags_and_sku_order(t *testing.T) {
	s := NewStore()
	fill(s)
	b, err := s.JSON()
	if err != nil {
		t.Fatalf("JSON() error: %v", err)
	}
	want := `[{"sku":"apple","qty":10,"price":90},{"sku":"fig","qty":5,"price":300},{"sku":"pear","qty":5,"price":120},{"sku":"plum","qty":0}]`
	if string(b) != want {
		t.Errorf("JSON() =\n%s\nwant\n%s", b, want)
	}
	empty, _ := NewStore().JSON()
	if string(empty) != "[]" {
		t.Errorf("JSON() of an empty store = %s; want []", empty)
	}
}

func TestLoggedStore_logs_removals_and_promotes_add(t *testing.T) {
	l := NewLoggedStore()
	l.Add(Item{"fig", 5, 300})
	if err := l.Remove("fig", 2); err != nil {
		t.Fatalf("Remove(fig, 2) = %v", err)
	}
	err := l.Remove("fig", 9)
	if !errors.Is(err, ErrInsufficient) {
		t.Errorf("Remove(fig, 9) = %v; want an error wrapping ErrInsufficient", err)
	}
	err2 := l.Remove("kiwi", 1)
	if !errors.Is(err2, ErrUnknownSKU) {
		t.Errorf("Remove(kiwi, 1) = %v; want an error wrapping ErrUnknownSKU", err2)
	}
	if len(l.Log) != 3 {
		t.Fatalf("Log = %q; want 3 entries", l.Log)
	}
	if l.Log[0] != "removed 2 fig" {
		t.Errorf("Log[0] = %q; want \"removed 2 fig\"", l.Log[0])
	}
	if want := "failed fig: " + err.Error(); l.Log[1] != want {
		t.Errorf("Log[1] = %q; want %q", l.Log[1], want)
	}
	if want := "failed kiwi: " + err2.Error(); l.Log[2] != want {
		t.Errorf("Log[2] = %q; want %q", l.Log[2], want)
	}
	if got := l.Where(func(it Item) bool { return true }); len(got) != 1 || got[0].Qty != 3 {
		t.Errorf("stock after the removals = %v; want fig with qty 3", got)
	}
}
