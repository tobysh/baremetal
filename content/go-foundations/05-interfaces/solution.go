package main

import (
	"fmt"
	"math"
)

type Shape interface {
	Area() float64
	Perimeter() float64
}

type Rect struct{ W, H float64 }
type Circle struct{ R float64 }

func (r Rect) Area() float64        { return r.W * r.H }
func (r Rect) Perimeter() float64   { return 2 * (r.W + r.H) }
func (c Circle) Area() float64      { return math.Pi * c.R * c.R }
func (c Circle) Perimeter() float64 { return 2 * math.Pi * c.R }

// Largest returns the shape with the biggest area (nil if there are none).
func Largest(shapes []Shape) Shape {
	var best Shape
	for _, s := range shapes {
		if best == nil || s.Area() > best.Area() {
			best = s
		}
	}
	return best
}

// Describe names the concrete shape: "rect 3x4", "circle r=2.5" or "unknown".
func Describe(s Shape) string {
	switch v := s.(type) {
	case Rect:
		return fmt.Sprintf("rect %vx%v", v.W, v.H)
	case Circle:
		return fmt.Sprintf("circle r=%v", v.R)
	default:
		return "unknown"
	}
}
