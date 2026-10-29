package main

import (
	"fmt"
	"net/http"
	"net/http/httptest"
)

type Middleware func(http.Handler) http.Handler

func Chain(h http.Handler, mws ...Middleware) http.Handler {
	return h
}

func RequireToken(token string) Middleware {
	return func(next http.Handler) http.Handler { return next }
}

func SetHeader(key, value string) Middleware {
	return func(next http.Handler) http.Handler { return next }
}

func MaxBody(n int64) Middleware {
	return func(next http.Handler) http.Handler { return next }
}

// Try it with Run.
func main() {
	h := Chain(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		fmt.Fprint(w, "secret")
	}), SetHeader("X-App", "demo"), RequireToken("s3"))
	rec := httptest.NewRecorder()
	h.ServeHTTP(rec, httptest.NewRequest("GET", "/", nil))
	fmt.Println(rec.Code, rec.Header().Get("X-App"))
}
