package main

import (
	"math"
	"testing"
)

func near(a, b float64) bool { return math.Abs(a-b) < 1e-9 }

func TestRect_area_and_perimeter(t *testing.T) {
	r := Rect{3, 4}
	if !near(r.Area(), 12) {
		t.Errorf("Rect{3, 4}.Area() = %v; want 12", r.Area())
	}
	if !near(r.Perimeter(), 14) {
		t.Errorf("Rect{3, 4}.Perimeter() = %v; want 14", r.Perimeter())
	}
}

func TestCircle_area_and_perimeter(t *testing.T) {
	c := Circle{2}
	if !near(c.Area(), 4*math.Pi) {
		t.Errorf("Circle{2}.Area() = %v; want %v", c.Area(), 4*math.Pi)
	}
	if !near(c.Perimeter(), 4*math.Pi) {
		t.Errorf("Circle{2}.Perimeter() = %v; want %v", c.Perimeter(), 4*math.Pi)
	}
}

func TestLargest_picks_biggest_area(t *testing.T) {
	shapes := []Shape{Rect{2, 2}, Circle{2}, Rect{3, 3}}
	got := Largest(shapes)
	c, ok := got.(Circle)
	if !ok || c.R != 2 {
		t.Errorf("Largest = %#v; want Circle{R: 2} (area %.2f)", got, math.Pi*4)
	}
}

func TestLargest_empty_is_nil(t *testing.T) {
	if got := Largest(nil); got != nil {
		t.Errorf("Largest(nil) = %#v; want nil", got)
	}
}

func TestLargest_single(t *testing.T) {
	got := Largest([]Shape{Rect{1, 5}})
	if r, ok := got.(Rect); !ok || r.W != 1 || r.H != 5 {
		t.Errorf("Largest of one shape = %#v; want that shape", got)
	}
}

type triangle struct{}

func (triangle) Area() float64      { return 1 }
func (triangle) Perimeter() float64 { return 3 }

func TestDescribe_each_kind(t *testing.T) {
	cases := []struct {
		s    Shape
		want string
	}{
		{Rect{3, 4}, "rect 3x4"},
		{Circle{2.5}, "circle r=2.5"},
		{triangle{}, "unknown"},
	}
	for _, c := range cases {
		if got := Describe(c.s); got != c.want {
			t.Errorf("Describe(%#v) = %q; want %q", c.s, got, c.want)
		}
	}
}
