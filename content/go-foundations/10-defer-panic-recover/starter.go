package main

// SafeDivide returns a / b, turning a divide-by-zero panic into an error.
func SafeDivide(a, b int) (result int, err error) {
	return 0, nil
}

// Try runs f and converts a panic into an error.
func Try(f func()) error {
	return nil
}

// DeferOrder defers one append per name and returns the order they ran.
func DeferOrder(names ...string) (ran []string) {
	return nil
}

// WithCleanup runs work, then always cleanup.
func WithCleanup(work func() error, cleanup func()) error {
	return nil
}
