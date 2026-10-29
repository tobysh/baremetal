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
	return func(next http.Handler) http.Handler {
		return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			ctx, cancel := context.WithTimeout(r.Context(), d)
			defer cancel()
			next.ServeHTTP(w, r.WithContext(ctx))
		})
	}
}

type idKey struct{}

func RequestID(gen func() string) Middleware {
	return func(next http.Handler) http.Handler {
		return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			id := r.Header.Get("X-Request-ID")
			if id == "" {
				id = gen()
			}
			w.Header().Set("X-Request-ID", id)
			next.ServeHTTP(w, r.WithContext(context.WithValue(r.Context(), idKey{}, id)))
		})
	}
}

func IDFrom(ctx context.Context) string {
	id, _ := ctx.Value(idKey{}).(string)
	return id
}

func Lookup(find func(ctx context.Context, key string) (string, error)) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodGet {
			w.Header().Set("Allow", "GET")
			http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
			return
		}
		key := r.URL.Query().Get("key")
		if key == "" {
			http.Error(w, "missing key", http.StatusBadRequest)
			return
		}
		v, err := find(r.Context(), key)
		switch {
		case err == nil:
			fmt.Fprint(w, v)
		case errors.Is(err, ErrNotFound):
			http.Error(w, "not found", http.StatusNotFound)
		case errors.Is(err, context.DeadlineExceeded):
			http.Error(w, "timeout", http.StatusGatewayTimeout)
		case errors.Is(err, context.Canceled):
			http.Error(w, "client closed request", 499)
		default:
			http.Error(w, "internal error", http.StatusInternalServerError)
		}
	})
}

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
