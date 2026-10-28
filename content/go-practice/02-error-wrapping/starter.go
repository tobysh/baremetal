package main

import (
	"errors"
	"fmt"
)

var ErrNotFound = errors.New("TODO")

type ValidationError struct {
	Field  string
	Reason string
}

func (e *ValidationError) Error() string {
	return ""
}

func Lookup(db map[string]int, key string) (int, error) {
	return 0, nil
}

func Validate(name string, age int) error {
	return nil
}

func Describe(err error) string {
	return ""
}

// Try it with Run.
func main() {
	_, err := Lookup(map[string]int{"a": 1}, "b")
	fmt.Println(err, "->", Describe(err))
	fmt.Println(Describe(Validate("", 3)))
}
