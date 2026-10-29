package main

import (
	"fmt"
	"net/http"
	"net/http/httptest"
)

type Middleware func(http.Handler) http.Handler

type statusWriter struct {
	http.ResponseWriter
	status int
	bytes  int
	wrote  bool
}

func (w *statusWriter) WriteHeader(code int) {
}

func (w *statusWriter) Write(b []byte) (int, error) {
	return 0, nil
}

func (w *statusWriter) Unwrap() http.ResponseWriter {
	return nil
}

func AccessLog(log func(method, path string, status, bytes int)) Middleware {
	return func(next http.Handler) http.Handler { return next }
}

func Recover(report func(v any)) Middleware {
	return func(next http.Handler) http.Handler { return next }
}

// Try it with Run.
func main() {
	var h http.Handler = http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		panic("oops")
	})
	h = Recover(func(v any) { fmt.Println("recovered:", v) })(h)
	h = AccessLog(func(m, p string, s, b int) { fmt.Println(m, p, s, b) })(h)
	rec := httptest.NewRecorder()
	h.ServeHTTP(rec, httptest.NewRequest("GET", "/boom", nil))
	fmt.Println(rec.Code, rec.Body.String())
}
