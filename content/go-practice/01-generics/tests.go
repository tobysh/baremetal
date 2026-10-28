package main

import (
	"reflect"
	"strconv"
	"strings"
	"testing"
)

func TestMap_changes_the_type(t *testing.T) {
	got := Map([]int{1, 2, 3}, strconv.Itoa)
	if !reflect.DeepEqual(got, []string{"1", "2", "3"}) {
		t.Errorf("Map([1 2 3], Itoa) = %q; want [1 2 3] as strings", got)
	}
	up := Map([]string{"a", "b"}, strings.ToUpper)
	if !reflect.DeepEqual(up, []string{"A", "B"}) {
		t.Errorf("Map(ToUpper) = %q; want [A B]", up)
	}
}

func TestMap_empty_is_not_nil(t *testing.T) {
	got := Map([]int{}, func(n int) int { return n })
	if got == nil || len(got) != 0 {
		t.Errorf("Map of an empty slice = %#v; want an empty, non-nil slice", got)
	}
}

func TestFilter_keeps_order(t *testing.T) {
	got := Filter([]int{5, 2, 8, 1, 9, 4}, func(n int) bool { return n%2 == 0 })
	if !reflect.DeepEqual(got, []int{2, 8, 4}) {
		t.Errorf("Filter(even) = %v; want [2 8 4]", got)
	}
	none := Filter([]string{"a"}, func(string) bool { return false })
	if none == nil || len(none) != 0 {
		t.Errorf("Filter keeping nothing = %#v; want an empty, non-nil slice", none)
	}
}

func TestReduce_sum_and_other_accumulator_types(t *testing.T) {
	sum := Reduce([]int{1, 2, 3, 4}, 0, func(a, n int) int { return a + n })
	if sum != 10 {
		t.Errorf("Reduce sum = %d; want 10", sum)
	}
	joined := Reduce([]int{1, 2, 3}, "", func(a string, n int) string { return a + strconv.Itoa(n) })
	if joined != "123" {
		t.Errorf("Reduce building a string = %q; want \"123\" (the fold must go left to right)", joined)
	}
	if Reduce([]int{}, 42, func(a, n int) int { return a + n }) != 42 {
		t.Errorf("Reduce of an empty slice must return init")
	}
}

func TestMax_ints_floats_and_strings(t *testing.T) {
	if m, ok := Max([]int{3, 9, 2}); !ok || m != 9 {
		t.Errorf("Max ints = %v, %v; want 9, true", m, ok)
	}
	if m, ok := Max([]float64{-2.5, -1.5}); !ok || m != -1.5 {
		t.Errorf("Max floats = %v, %v; want -1.5, true", m, ok)
	}
	if m, ok := Max([]string{"pear", "apple", "zebra", "mango"}); !ok || m != "zebra" {
		t.Errorf("Max strings = %q, %v; want \"zebra\", true", m, ok)
	}
}

func TestMax_empty(t *testing.T) {
	if m, ok := Max([]int{}); ok || m != 0 {
		t.Errorf("Max of an empty slice = %v, %v; want 0, false", m, ok)
	}
}

func TestStack_last_in_first_out(t *testing.T) {
	var s Stack[string]
	if _, ok := s.Pop(); ok {
		t.Errorf("Pop on a new Stack returned ok = true")
	}
	s.Push("a")
	s.Push("b")
	s.Push("c")
	if s.Len() != 3 {
		t.Errorf("Len = %d; want 3", s.Len())
	}
	for _, want := range []string{"c", "b", "a"} {
		got, ok := s.Pop()
		if !ok || got != want {
			t.Errorf("Pop = %q, %v; want %q, true", got, ok, want)
		}
	}
	if s.Len() != 0 {
		t.Errorf("Len after popping everything = %d; want 0", s.Len())
	}
}

func TestStack_works_for_other_types(t *testing.T) {
	var s Stack[[]int]
	s.Push([]int{1})
	s.Push([]int{2, 3})
	if got, _ := s.Pop(); !reflect.DeepEqual(got, []int{2, 3}) {
		t.Errorf("Pop = %v; want [2 3]", got)
	}
}
