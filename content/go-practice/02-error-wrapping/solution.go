package main

import (
	"errors"
	"fmt"
)

var ErrNotFound = errors.New("not found")

type ValidationError struct {
	Field  string
	Reason string
}

func (e *ValidationError) Error() string {
	return e.Field + ": " + e.Reason
}

func Lookup(db map[string]int, key string) (int, error) {
	v, ok := db[key]
	if !ok {
		return 0, fmt.Errorf("lookup %q: %w", key, ErrNotFound)
	}
	return v, nil
}

func Validate(name string, age int) error {
	if name == "" {
		return &ValidationError{Field: "name", Reason: "must not be empty"}
	}
	if age < 0 || age > 150 {
		return &ValidationError{Field: "age", Reason: "must be between 0 and 150"}
	}
	return nil
}

func Describe(err error) string {
	if err == nil {
		return "ok"
	}
	if errors.Is(err, ErrNotFound) {
		return "missing"
	}
	var ve *ValidationError
	if errors.As(err, &ve) {
		return "invalid " + ve.Field + " (" + ve.Reason + ")"
	}
	return "error: " + err.Error()
}

// Try it with Run.
func main() {
	_, err := Lookup(map[string]int{"a": 1}, "b")
	fmt.Println(err, "->", Describe(err))
	fmt.Println(Describe(Validate("", 3)))
}
