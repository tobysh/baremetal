package main

type Shape interface {
	Area() float64
	Perimeter() float64
}

type Rect struct{ W, H float64 }
type Circle struct{ R float64 }

func (r Rect) Area() float64        { return 0 }
func (r Rect) Perimeter() float64   { return 0 }
func (c Circle) Area() float64      { return 0 }
func (c Circle) Perimeter() float64 { return 0 }

// Largest returns the shape with the biggest area (nil if there are none).
func Largest(shapes []Shape) Shape {
	return nil
}

// Describe names the concrete shape: "rect 3x4", "circle r=2.5" or "unknown".
func Describe(s Shape) string {
	return ""
}
