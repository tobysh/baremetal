package main

// Counter counts. Inc must change the caller's counter.
type Counter struct{ n int }

func (c *Counter) Inc() { c.n++ }

func (c *Counter) Value() int { return c.n }

// IncAll increments every counter in cs.
func IncAll(cs []Counter) {
	for i := range cs {
		cs[i].Inc()
	}
}

// Rect is a rectangle.
type Rect struct{ W, H float64 }

func (r Rect) Area() float64 { return r.W * r.H }

// Scale multiplies both sides by f.
func (r *Rect) Scale(f float64) {
	r.W *= f
	r.H *= f
}

// Scaler is anything that can be scaled in place.
type Scaler interface{ Scale(f float64) }

// Grow scales every item by f.
func Grow(items []Scaler, f float64) {
	for _, it := range items {
		it.Scale(f)
	}
}

// Stack is a last-in, first-out stack of ints.
type Stack struct{ items []int }

func (s *Stack) Push(v int) { s.items = append(s.items, v) }

func (s *Stack) Pop() (int, bool) {
	if len(s.items) == 0 {
		return 0, false
	}
	v := s.items[len(s.items)-1]
	s.items = s.items[:len(s.items)-1]
	return v, true
}

func (s *Stack) Len() int { return len(s.items) }

// Pusher is anything that accepts pushed ints.
type Pusher interface{ Push(int) }

// Fill pushes 1..n onto p.
func Fill(p Pusher, n int) {
	for i := 1; i <= n; i++ {
		p.Push(i)
	}
}
