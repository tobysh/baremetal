package main

import (
	"reflect"
	"testing"
)

func TestSortPeople_by_age_then_name(t *testing.T) {
	ps := []Person{{"Bo", 31}, {"Al", 31}, {"Cy", 25}, {"Di", 40}, {"Ab", 31}}
	SortPeople(ps)
	want := []Person{{"Cy", 25}, {"Ab", 31}, {"Al", 31}, {"Bo", 31}, {"Di", 40}}
	if !reflect.DeepEqual(ps, want) {
		t.Errorf("SortPeople = %v; want %v", ps, want)
	}
}

func TestSortPeople_empty_and_single(t *testing.T) {
	SortPeople(nil)
	one := []Person{{"Z", 1}}
	SortPeople(one)
	if one[0].Name != "Z" {
		t.Errorf("single element changed: %v", one)
	}
}

func TestTopWords_ranks_by_count_then_alphabet(t *testing.T) {
	words := []string{"pear", "fig", "pear", "apple", "fig", "pear", "kiwi", "apple"}
	got := TopWords(words, 3)
	want := []string{"pear", "apple", "fig"}
	if !reflect.DeepEqual(got, want) {
		t.Errorf("TopWords(.., 3) = %v; want %v (pear 3, then apple 2 and fig 2 alphabetically)", got, want)
	}
}

func TestTopWords_n_out_of_range(t *testing.T) {
	words := []string{"b", "a", "b"}
	if got := TopWords(words, 10); !reflect.DeepEqual(got, []string{"b", "a"}) {
		t.Errorf("TopWords(.., 10) = %v; want [b a]", got)
	}
	for _, n := range []int{0, -2} {
		got := TopWords(words, n)
		if got == nil || len(got) != 0 {
			t.Errorf("TopWords(.., %d) = %#v; want an empty, non-nil slice", n, got)
		}
	}
	if got := TopWords(nil, 3); len(got) != 0 {
		t.Errorf("TopWords(nil, 3) = %v; want empty", got)
	}
}

func TestCounter_counts_up_independently(t *testing.T) {
	a, b := Counter(), Counter()
	if a() != 1 || a() != 2 || a() != 3 {
		t.Errorf("first counter should give 1, 2, 3")
	}
	if got := b(); got != 1 {
		t.Errorf("a second Counter() starts at 1, got %d (is the count shared between counters?)", got)
	}
	if got := a(); got != 4 {
		t.Errorf("first counter's 4th call = %d; want 4", got)
	}
}

func TestMemoize_calls_once_per_argument(t *testing.T) {
	calls := 0
	slowSquare := func(n int) int { calls++; return n * n }
	f := Memoize(slowSquare)
	for _, n := range []int{3, 4, 3, 3, 4, 5} {
		if got := f(n); got != n*n {
			t.Errorf("f(%d) = %d; want %d", n, got, n*n)
		}
	}
	if calls != 3 {
		t.Errorf("the wrapped function ran %d times for 3 distinct arguments; want 3", calls)
	}
}

func TestMemoize_caches_zero_results_too(t *testing.T) {
	calls := 0
	f := Memoize(func(n int) int { calls++; return 0 })
	f(1)
	f(1)
	if calls != 1 {
		t.Errorf("a result of 0 must be cached too; the function ran %d times for one argument", calls)
	}
}
