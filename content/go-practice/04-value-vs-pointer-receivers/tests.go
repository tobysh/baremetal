package main

import (
	"reflect"
	"testing"
)

// Rect must have Area in both method sets; *Rect must satisfy Scaler.
type areaer interface{ Area() float64 }

var _ areaer = Rect{}
var _ areaer = &Rect{}
var _ Scaler = &Rect{}
var _ Pusher = &Stack{}

func TestCounter_inc_changes_the_callers_counter(t *testing.T) {
	var c Counter
	for i := 0; i < 3; i++ {
		c.Inc()
	}
	if c.Value() != 3 {
		t.Errorf("after 3 calls to Inc, Value() = %d; want 3 (is Inc using a pointer receiver?)", c.Value())
	}
	p := &Counter{}
	p.Inc()
	if p.Value() != 1 {
		t.Errorf("Inc through a pointer: Value() = %d; want 1", p.Value())
	}
}

func TestIncAll_changes_the_slice_elements(t *testing.T) {
	cs := make([]Counter, 4)
	cs[1].Inc()
	IncAll(cs)
	IncAll(cs)
	got := []int{}
	for i := range cs {
		got = append(got, cs[i].Value())
	}
	if !reflect.DeepEqual(got, []int{2, 3, 2, 2}) {
		t.Errorf("values after Inc on cs[1] and IncAll twice = %v; want [2 3 2 2]", got)
	}
	IncAll(nil)
}

func TestRect_area(t *testing.T) {
	for _, c := range []struct{ w, h, want float64 }{{2, 3, 6}, {0, 5, 0}, {1.5, 4, 6}} {
		if got := (Rect{c.w, c.h}).Area(); got != c.want {
			t.Errorf("Rect{%v, %v}.Area() = %v; want %v", c.w, c.h, got, c.want)
		}
	}
}

func TestRect_scale_in_place(t *testing.T) {
	r := Rect{2, 3}
	r.Scale(2)
	if r.W != 4 || r.H != 6 {
		t.Errorf("Rect{2, 3} after Scale(2) = %v; want {4 6}", r)
	}
	r.Scale(0.5)
	if r.W != 2 || r.H != 3 {
		t.Errorf("then Scale(0.5) = %v; want {2 3}", r)
	}
}

func TestGrow_scales_all_items(t *testing.T) {
	a, b := &Rect{1, 2}, &Rect{3, 4}
	Grow([]Scaler{a, b}, 3)
	if *a != (Rect{3, 6}) || *b != (Rect{9, 12}) {
		t.Errorf("after Grow(..., 3): %v and %v; want {3 6} and {9 12}", *a, *b)
	}
	Grow(nil, 2)
}

func TestStack_push_pop_len(t *testing.T) {
	var s Stack
	if v, ok := s.Pop(); ok || v != 0 {
		t.Errorf("Pop on an empty stack = %d, %v; want 0, false", v, ok)
	}
	s.Push(10)
	s.Push(20)
	s.Push(30)
	if s.Len() != 3 {
		t.Errorf("Len after 3 pushes = %d; want 3", s.Len())
	}
	for _, want := range []int{30, 20, 10} {
		if v, ok := s.Pop(); !ok || v != want {
			t.Errorf("Pop = %d, %v; want %d, true", v, ok, want)
		}
	}
	if s.Len() != 0 {
		t.Errorf("Len after popping everything = %d; want 0", s.Len())
	}
}

func TestFill_pushes_one_to_n(t *testing.T) {
	var s Stack
	Fill(&s, 4)
	if s.Len() != 4 {
		t.Fatalf("after Fill(&s, 4), Len = %d; want 4", s.Len())
	}
	if v, _ := s.Pop(); v != 4 {
		t.Errorf("top of the stack = %d; want 4", v)
	}
	var empty Stack
	Fill(&empty, 0)
	Fill(&empty, -3)
	if empty.Len() != 0 {
		t.Errorf("Fill with n <= 0 pushed %d values; want none", empty.Len())
	}
}
