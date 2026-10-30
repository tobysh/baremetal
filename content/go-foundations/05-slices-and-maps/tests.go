package main

import (
	"reflect"
	"testing"
)

func TestWordCount_repeated_words(t *testing.T) {
	got := WordCount("the cat and the dog and the bird")
	want := map[string]int{"the": 3, "cat": 1, "and": 2, "dog": 1, "bird": 1}
	if !reflect.DeepEqual(got, want) {
		t.Errorf("WordCount(...) = %v; want %v", got, want)
	}
}

func TestWordCount_ignores_case(t *testing.T) {
	got := WordCount("Go go GO gO")
	if got["go"] != 4 || len(got) != 1 {
		t.Errorf("WordCount(\"Go go GO gO\") = %v; want map[go:4]", got)
	}
}

func TestWordCount_extra_whitespace(t *testing.T) {
	got := WordCount("  a \t b\n\na  ")
	want := map[string]int{"a": 2, "b": 1}
	if !reflect.DeepEqual(got, want) {
		t.Errorf("WordCount with extra whitespace = %v; want %v", got, want)
	}
}

func TestWordCount_empty_text_gives_non_nil_map(t *testing.T) {
	got := WordCount("")
	if got == nil {
		t.Fatalf("WordCount(\"\") returned a nil map; want an empty map you can write to")
	}
	if len(got) != 0 {
		t.Errorf("WordCount(\"\") = %v; want an empty map", got)
	}
}

func TestDedup_keeps_first_occurrence_in_order(t *testing.T) {
	got := Dedup([]int{3, 1, 3, 2, 1, 4, 2})
	want := []int{3, 1, 2, 4}
	if !reflect.DeepEqual(got, want) {
		t.Errorf("Dedup([3 1 3 2 1 4 2]) = %v; want %v", got, want)
	}
}

func TestDedup_does_not_modify_input(t *testing.T) {
	in := []int{5, 5, 6, 5}
	Dedup(in)
	if !reflect.DeepEqual(in, []int{5, 5, 6, 5}) {
		t.Errorf("Dedup changed its input to %v; it must leave it alone", in)
	}
}

func TestDedup_empty_input(t *testing.T) {
	got := Dedup(nil)
	if got == nil || len(got) != 0 {
		t.Errorf("Dedup(nil) = %#v; want an empty, non-nil slice []int{}", got)
	}
}

func TestDedup_all_same(t *testing.T) {
	got := Dedup([]int{7, 7, 7})
	if !reflect.DeepEqual(got, []int{7}) {
		t.Errorf("Dedup([7 7 7]) = %v; want [7]", got)
	}
}
