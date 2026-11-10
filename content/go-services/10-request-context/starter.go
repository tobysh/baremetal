package main

import (
	"context"
	"errors"
	"fmt"
	"net/http"
	"net/http/httptest"
	"time"
)

type Middleware func(http.Handler) http.Handler

var ErrNotFound = errors.New("not found")

func Timeout(d time.Duration) Middleware {
	return func(next http.Handler) http.Handler { return next }
}

func RequestID(gen func() string) Middleware {
	return func(next http.Handler) http.Handler { return next }
}

func IDFrom(ctx context.Context) string {
	return ""
}

func Lookup(find func(ctx context.Context, key string) (string, error)) http.Handler {
	return http.NotFoundHandler()
}

// Scaffolding supplied for you: main below only demonstrates your code.
// Your part is the stubbed declarations above; replace their bodies.
// Try it with Run.
func main() {
	h := Lookup(func(ctx context.Context, key string) (string, error) {
		if key == "a" {
			return "apple", nil
		}
		return "", ErrNotFound
	})
	for _, k := range []string{"a", "z", ""} {
		rec := httptest.NewRecorder()
		h.ServeHTTP(rec, httptest.NewRequest("GET", "/?key="+k, nil))
		fmt.Println(rec.Code, rec.Body.String())
	}
}
