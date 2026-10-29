package main

import "fmt"

// Clamp limits x to the range [lo, hi]. If lo > hi the bounds are swapped first.
func Clamp(x, lo, hi int) int {
	if lo > hi {
		lo, hi = hi, lo
	}
	if x < lo {
		return lo
	}
	if x > hi {
		return hi
	}
	return x
}

type ClampCase struct {
	Name      string
	X, Lo, Hi int
	Want      int
}

func ClampCases() []ClampCase {
	return []ClampCase{
		{"inside", 5, 1, 10, 5},
		{"below", -4, 1, 10, 1},
		{"above", 99, 1, 10, 10},
		{"equals lo", 1, 1, 10, 1},
		{"equals hi", 10, 1, 10, 10},
		{"lo equals hi, x above", 8, 3, 3, 3},
		{"lo equals hi, x same", 3, 3, 3, 3},
		{"negative inside", -3, -10, 10, -3},
		{"negative range below", -20, -10, -5, -10},
		{"negative range above", 0, -10, -5, -5},
		{"swapped, inside", 3, 5, 1, 3},
		{"swapped, below", 0, 5, 1, 1},
		{"swapped, above", 9, 5, 1, 5},
		{"zero", 0, -1, 1, 0},
	}
}

func CheckClamp(f func(x, lo, hi int) int, cases []ClampCase) []string {
	var fails []string
	for _, c := range cases {
		if got := f(c.X, c.Lo, c.Hi); got != c.Want {
			fails = append(fails, fmt.Sprintf("%s: Clamp(%d, %d, %d) = %d; want %d", c.Name, c.X, c.Lo, c.Hi, got, c.Want))
		}
	}
	return fails
}

func main() {
	fmt.Println(len(ClampCases()), "cases;", CheckClamp(Clamp, ClampCases()))
}
