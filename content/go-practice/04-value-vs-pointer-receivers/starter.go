package main

// Counter counts. Inc must change the caller's counter.
type Counter struct{ n int }

func (c Counter) Inc() {} // wrong receiver: this works on a copy

func (c Counter) Value() int { return c.n }

// IncAll increments every counter in cs.
func IncAll(cs []Counter) {
}

// Rect is a rectangle.
type Rect struct{ W, H float64 }

func (r Rect) Area() float64 { return 0 }

// Scale multiplies both sides by f.
func (r Rect) Scale(f float64) {}

// Scaler is anything that can be scaled in place.
type Scaler interface{ Scale(f float64) }

// Grow scales every item by f.
func Grow(items []Scaler, f float64) {
}

// Stack is a last-in, first-out stack of ints.
type Stack struct{ items []int }

func (s Stack) Push(v int) {}

func (s Stack) Pop() (int, bool) { return 0, false }

func (s Stack) Len() int { return len(s.items) }

// Pusher is anything that accepts pushed ints.
type Pusher interface{ Push(int) }

// Fill pushes 1..n onto p.
func Fill(p Pusher, n int) {
}
