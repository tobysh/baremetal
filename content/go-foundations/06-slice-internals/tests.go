package main

import (
	"reflect"
	"testing"
)

func TestSafeAppend_values(t *testing.T) {
	if got := SafeAppend(nil, 1); !reflect.DeepEqual(got, []int{1}) {
		t.Errorf("SafeAppend(nil, 1) = %v; want [1]", got)
	}
	if got := SafeAppend([]int{1, 2}, 3); !reflect.DeepEqual(got, []int{1, 2, 3}) {
		t.Errorf("SafeAppend([1 2], 3) = %v; want [1 2 3]", got)
	}
}

func TestSafeAppend_does_not_share_spare_capacity(t *testing.T) {
	base := make([]int, 3, 10)
	a := SafeAppend(base, 1)
	b := SafeAppend(base, 2)
	if len(a) != 4 || len(b) != 4 || a[3] != 1 || b[3] != 2 {
		t.Errorf("SafeAppend(base, 1) = %v and SafeAppend(base, 2) = %v; want [0 0 0 1] and [0 0 0 2]", a, b)
	}
	if spare := base[:4]; spare[3] != 0 {
		t.Errorf("SafeAppend wrote into base's spare capacity (slot 3 is %d, want 0)", spare[3])
	}
}

func TestClone_is_independent(t *testing.T) {
	orig := []int{1, 2, 3}
	c := Clone(orig)
	if !reflect.DeepEqual(c, orig) {
		t.Fatalf("Clone([1 2 3]) = %v", c)
	}
	c[0] = 100
	if orig[0] != 1 {
		t.Errorf("changing the clone changed the original: %v", orig)
	}
	orig[1] = 200
	if c[1] != 2 {
		t.Errorf("changing the original changed the clone: %v", c)
	}
}

func TestClone_nil_and_empty(t *testing.T) {
	if got := Clone(nil); got != nil {
		t.Errorf("Clone(nil) = %#v; want nil", got)
	}
	if got := Clone([]int{}); got == nil || len(got) != 0 {
		t.Errorf("Clone([]int{}) = %#v; want an empty non-nil slice", got)
	}
}

func TestWindow_shares_elements(t *testing.T) {
	orig := []int{1, 2, 3, 4, 5}
	w := Window(orig, 1, 3)
	if !reflect.DeepEqual(w, []int{2, 3}) {
		t.Fatalf("Window([1 2 3 4 5], 1, 3) = %v; want [2 3]", w)
	}
	w[0] = 100
	if orig[1] != 100 {
		t.Errorf("a window should share elements with the original; orig = %v", orig)
	}
}

func TestWindow_append_cannot_overwrite_original(t *testing.T) {
	for _, c := range []struct{ from, to int }{{1, 3}, {0, 2}, {2, 4}} {
		orig := []int{1, 2, 3, 4, 5}
		w := Window(orig, c.from, c.to)
		_ = append(w, 99)
		if !reflect.DeepEqual(orig, []int{1, 2, 3, 4, 5}) {
			t.Errorf("Window(orig, %d, %d) then append changed orig to %v; cap(w) is %d, want %d",
				c.from, c.to, orig, cap(w), c.to-c.from)
		}
	}
}

func TestRemoveAt_values(t *testing.T) {
	for _, c := range []struct {
		in   []int
		i    int
		want []int
	}{
		{[]int{1, 2, 3, 4}, 0, []int{2, 3, 4}},
		{[]int{1, 2, 3, 4}, 2, []int{1, 2, 4}},
		{[]int{1, 2, 3, 4}, 3, []int{1, 2, 3}},
		{[]int{7}, 0, []int{}},
	} {
		got := RemoveAt(append([]int(nil), c.in...), c.i)
		if !reflect.DeepEqual(got, c.want) {
			t.Errorf("RemoveAt(%v, %d) = %v; want %v", c.in, c.i, got, c.want)
		}
	}
}

func TestRemoveAt_works_in_place(t *testing.T) {
	s := []int{1, 2, 3, 4}
	got := RemoveAt(s, 1)
	if len(got) != 3 || &got[0] != &s[0] {
		t.Errorf("RemoveAt should reuse the same array")
	}
	if !reflect.DeepEqual(s[:3], []int{1, 3, 4}) {
		t.Errorf("after RemoveAt(s, 1) the front of s should be [1 3 4], got %v", s[:3])
	}
}
