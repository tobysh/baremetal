package main

import (
	"errors"
	"fmt"
)

// SafeDivide returns a / b, turning a divide-by-zero panic into an error.
func SafeDivide(a, b int) (result int, err error) {
	defer func() {
		if r := recover(); r != nil {
			err = fmt.Errorf("recovered: %v", r)
		}
	}()
	return a / b, nil
}

// Try runs f and converts a panic into an error.
func Try(f func()) (err error) {
	defer func() {
		if r := recover(); r != nil {
			switch v := r.(type) {
			case error:
				err = v
			case string:
				err = errors.New(v)
			default:
				err = fmt.Errorf("panic: %v", v)
			}
		}
	}()
	f()
	return nil
}

// DeferOrder defers one append per name and returns the order they ran.
func DeferOrder(names ...string) (ran []string) {
	for _, n := range names {
		defer func() { ran = append(ran, n) }()
	}
	return
}

// WithCleanup runs work, then always cleanup.
func WithCleanup(work func() error, cleanup func()) error {
	defer cleanup()
	return work()
}
