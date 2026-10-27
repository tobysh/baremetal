package main

import (
	"reflect"
	"strings"
	"testing"
)

func top(text string, n int) []WordCount { return TopWords(strings.NewReader(text), n) }

func TestTopWords_ranks_by_count(t *testing.T) {
	got := top("a b b c c c", 3)
	want := []WordCount{{"c", 3}, {"b", 2}, {"a", 1}}
	if !reflect.DeepEqual(got, want) {
		t.Errorf("TopWords(\"a b b c c c\", 3) = %v; want %v", got, want)
	}
}

func TestTopWords_ties_are_alphabetical(t *testing.T) {
	got := top("pear apple fig apple pear fig kiwi", 4)
	want := []WordCount{{"apple", 2}, {"fig", 2}, {"pear", 2}, {"kiwi", 1}}
	if !reflect.DeepEqual(got, want) {
		t.Errorf("tie order = %v; want %v", got, want)
	}
}

func TestTopWords_ignores_case_and_punctuation(t *testing.T) {
	got := top("Go, go! GO? Gopher... gopher; \"Go\".", 5)
	want := []WordCount{{"go", 4}, {"gopher", 2}}
	if !reflect.DeepEqual(got, want) {
		t.Errorf("TopWords = %v; want %v", got, want)
	}
}

func TestTopWords_keeps_apostrophes_inside_words(t *testing.T) {
	got := top("Don't stop. don't DON'T!", 1)
	want := []WordCount{{"don't", 3}}
	if !reflect.DeepEqual(got, want) {
		t.Errorf("TopWords = %v; want %v", got, want)
	}
}

func TestTopWords_limits_to_n(t *testing.T) {
	got := top("a a a b b c d", 2)
	want := []WordCount{{"a", 3}, {"b", 2}}
	if !reflect.DeepEqual(got, want) {
		t.Errorf("TopWords(..., 2) = %v; want %v", got, want)
	}
}

func TestTopWords_spans_several_lines(t *testing.T) {
	got := top("one two\ntwo three\n\nthree three\n", 10)
	want := []WordCount{{"three", 3}, {"two", 2}, {"one", 1}}
	if !reflect.DeepEqual(got, want) {
		t.Errorf("TopWords = %v; want %v", got, want)
	}
}

func TestTopWords_nothing_to_count(t *testing.T) {
	for _, c := range []struct {
		text string
		n    int
	}{{"", 3}, {"... !!! 123", 3}, {"word", 0}, {"word", -1}} {
		got := top(c.text, c.n)
		if got == nil || len(got) != 0 {
			t.Errorf("TopWords(%q, %d) = %#v; want an empty, non-nil slice", c.text, c.n, got)
		}
	}
}
