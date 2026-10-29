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
	return nil
}

func CheckClamp(f func(x, lo, hi int) int, cases []ClampCase) []string {
	return nil
}

// Try it with Run.
func main() {
	fmt.Println(len(ClampCases()), "cases;", CheckClamp(Clamp, ClampCases()))
}
