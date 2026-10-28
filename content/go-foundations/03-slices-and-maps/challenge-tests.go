package main

import (
	"reflect"
	"testing"
)

func TestGroupAnagrams_groups_in_first_seen_order(t *testing.T) {
	got := GroupAnagrams([]string{"listen", "google", "silent", "gogole", "enlist"})
	want := [][]string{{"listen", "silent", "enlist"}, {"google", "gogole"}}
	if !reflect.DeepEqual(got, want) {
		t.Errorf("GroupAnagrams = %v; want %v", got, want)
	}
}

func TestGroupAnagrams_singletons_and_duplicates(t *testing.T) {
	got := GroupAnagrams([]string{"abc", "xyz", "cab", "abc"})
	want := [][]string{{"abc", "cab", "abc"}, {"xyz"}}
	if !reflect.DeepEqual(got, want) {
		t.Errorf("GroupAnagrams = %v; want %v", got, want)
	}
}

func TestGroupAnagrams_is_case_sensitive(t *testing.T) {
	got := GroupAnagrams([]string{"Ab", "bA", "ab"})
	want := [][]string{{"Ab", "bA"}, {"ab"}}
	if !reflect.DeepEqual(got, want) {
		t.Errorf("GroupAnagrams = %v; want %v (compare letters exactly as given)", got, want)
	}
}

func TestGroupAnagrams_empty(t *testing.T) {
	got := GroupAnagrams(nil)
	if got == nil || len(got) != 0 {
		t.Errorf("GroupAnagrams(nil) = %#v; want an empty, non-nil slice", got)
	}
}

func TestGroupAnagrams_unicode_and_empty_words(t *testing.T) {
	got := GroupAnagrams([]string{"é1", "1é", "", ""})
	want := [][]string{{"é1", "1é"}, {"", ""}}
	if !reflect.DeepEqual(got, want) {
		t.Errorf("GroupAnagrams = %q; want %q (work with runes, not bytes)", got, want)
	}
}
