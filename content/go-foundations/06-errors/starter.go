package main

type RangeError struct {
	Value, Min, Max int
}

func (e *RangeError) Error() string {
	return ""
}

// ErrDivideByZero should be a sentinel error value (see errors.New).
var ErrDivideByZero error

// Divide returns ErrDivideByZero when b is 0.
func Divide(a, b float64) (float64, error) {
	return 0, nil
}

// ParseAge converts s to an age in 0..150.
func ParseAge(s string) (int, error) {
	return 0, nil
}
