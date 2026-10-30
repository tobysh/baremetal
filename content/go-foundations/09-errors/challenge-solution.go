package main

import (
	"errors"
	"fmt"
	"strconv"
)

type RangeError struct {
	Value, Min, Max int
}

func (e *RangeError) Error() string {
	return fmt.Sprintf("%d is outside %d..%d", e.Value, e.Min, e.Max)
}

// ErrDivideByZero should be a sentinel error value (see errors.New).
var ErrDivideByZero = errors.New("divide by zero")

// Divide returns ErrDivideByZero when b is 0.
func Divide(a, b float64) (float64, error) {
	if b == 0 {
		return 0, ErrDivideByZero
	}
	return a / b, nil
}

// ParseAge converts s to an age in 0..150.
func ParseAge(s string) (int, error) {
	n, err := strconv.Atoi(s)
	if err != nil {
		return 0, fmt.Errorf("parse age: %w", err)
	}
	if n < 0 || n > 150 {
		return 0, &RangeError{Value: n, Min: 0, Max: 150}
	}
	return n, nil
}

// ParseAges parses every input and joins all the failures.
func ParseAges(inputs []string) ([]int, error) {
	var ages []int
	var errs []error
	for i, s := range inputs {
		age, err := ParseAge(s)
		if err != nil {
			errs = append(errs, fmt.Errorf("input %d: %w", i, err))
			continue
		}
		ages = append(ages, age)
	}
	return ages, errors.Join(errs...)
}

// RangeErrors returns every *RangeError found anywhere inside err.
func RangeErrors(err error) []*RangeError {
	if err == nil {
		return nil
	}
	var found []*RangeError
	if re, ok := err.(*RangeError); ok {
		found = append(found, re)
	}
	switch u := err.(type) {
	case interface{ Unwrap() []error }:
		for _, e := range u.Unwrap() {
			found = append(found, RangeErrors(e)...)
		}
	case interface{ Unwrap() error }:
		found = append(found, RangeErrors(u.Unwrap())...)
	}
	return found
}
