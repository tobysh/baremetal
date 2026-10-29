package main

import (
	"fmt"
	"net/http"
	"net/http/httptest"
	"testing"
)

func DoJSON(h http.Handler, method, target string, body any) *httptest.ResponseRecorder {
	return nil
}

func DecodeBody[T any](t testing.TB, rec *httptest.ResponseRecorder) T {
	var zero T
	return zero
}

func AssertStatus(t testing.TB, rec *httptest.ResponseRecorder, want int) {
}

func AssertJSONError(t testing.TB, rec *httptest.ResponseRecorder, status int, msg string) {
}

// Try it with Run. (Checks need a testing.TB, so the hidden tests use a fake one.)
func main() {
	h := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { fmt.Fprint(w, `{"ok":true}`) })
	rec := DoJSON(h, "GET", "/", nil)
	fmt.Println(rec != nil)
}
