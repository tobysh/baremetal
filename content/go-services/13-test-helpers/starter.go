package main

import (
	"fmt"
	"testing"
)

func AssertEqual[T comparable](t testing.TB, got, want T) {
}

func AssertNoError(t testing.TB, err error) {
}

func AssertErrorIs(t testing.TB, err, target error) {
}

func WithEnv(t testing.TB, key, value string) {
}

func TempFile(t testing.TB, name, content string) string {
	return ""
}

// Try it with Run. (The real checks need a testing.TB, so the hidden tests supply a fake one.)
func main() {
	fmt.Println("write the helpers, then press Test")
}
