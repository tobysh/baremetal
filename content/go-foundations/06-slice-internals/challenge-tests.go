package main

import (
	"reflect"
	"testing"
)

func TestChunk_values(t *testing.T) {
	for _, c := range []struct {
		in   []int
		n    int
		want [][]int
	}{
		{[]int{1, 2, 3, 4, 5}, 2, [][]int{{1, 2}, {3, 4}, {5}}},
		{[]int{1, 2, 3, 4}, 2, [][]int{{1, 2}, {3, 4}}},
		{[]int{1, 2, 3}, 5, [][]int{{1, 2, 3}}},
		{[]int{1, 2, 3}, 1, [][]int{{1}, {2}, {3}}},
		{[]int{9, 8, 7, 6, 5, 4, 3}, 3, [][]int{{9, 8, 7}, {6, 5, 4}, {3}}},
	} {
		got := Chunk(append([]int(nil), c.in...), c.n)
		if !reflect.DeepEqual(got, c.want) {
			t.Errorf("Chunk(%v, %d) = %v; want %v", c.in, c.n, got, c.want)
		}
	}
}

func TestChunk_empty_and_bad_size(t *testing.T) {
	for _, c := range []struct {
		in []int
		n  int
	}{{nil, 3}, {[]int{}, 2}, {[]int{1, 2}, 0}, {[]int{1, 2}, -1}} {
		got := Chunk(c.in, c.n)
		if got == nil || len(got) != 0 {
			t.Errorf("Chunk(%v, %d) = %#v; want an empty, non-nil slice", c.in, c.n, got)
		}
	}
}

func TestChunk_shares_elements(t *testing.T) {
	s := []int{1, 2, 3, 4, 5}
	got := Chunk(s, 2)
	if len(got) != 3 {
		t.Fatalf("Chunk(s, 2) has %d chunks; want 3", len(got))
	}
	got[1][0] = 100
	got[2][0] = 500
	if !reflect.DeepEqual(s, []int{1, 2, 100, 4, 500}) {
		t.Errorf("chunks should share memory with s; s = %v", s)
	}
}

func TestChunk_append_cannot_overwrite_neighbours(t *testing.T) {
	for _, n := range []int{1, 2, 3, 4} {
		s := []int{1, 2, 3, 4, 5, 6, 7}
		got := Chunk(s, n)
		for i, c := range got {
			if cap(c) != len(c) {
				t.Errorf("Chunk(s, %d)[%d]: cap is %d, want %d", n, i, cap(c), len(c))
			}
			_ = append(c, -1)
		}
		if !reflect.DeepEqual(s, []int{1, 2, 3, 4, 5, 6, 7}) {
			t.Errorf("Chunk(s, %d) then append changed s to %v", n, s)
		}
	}
}
