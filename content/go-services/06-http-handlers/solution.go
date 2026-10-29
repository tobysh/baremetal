package main

import (
	"fmt"
	"net/http"
	"net/http/httptest"
)

func Greet(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		w.Header().Set("Allow", "GET")
		http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
		return
	}
	name := r.URL.Query().Get("name")
	if name == "" {
		name = "world"
	}
	w.Header().Set("Content-Type", "text/plain; charset=utf-8")
	fmt.Fprintf(w, "Hello, %s!\n", name)
}

func allDigits(s string) bool {
	if s == "" {
		return false
	}
	for i := 0; i < len(s); i++ {
		if s[i] < '0' || s[i] > '9' {
			return false
		}
	}
	return true
}

func NewMux() *http.ServeMux {
	mux := http.NewServeMux()
	mux.HandleFunc("GET /greet", Greet)
	mux.HandleFunc("GET /health", func(w http.ResponseWriter, r *http.Request) {
		fmt.Fprint(w, "ok")
	})
	mux.HandleFunc("GET /users/{id}", func(w http.ResponseWriter, r *http.Request) {
		id := r.PathValue("id")
		if !allDigits(id) {
			http.Error(w, "bad id", http.StatusBadRequest)
			return
		}
		fmt.Fprintf(w, "user %s", id)
	})
	return mux
}

func main() {
	rec := httptest.NewRecorder()
	NewMux().ServeHTTP(rec, httptest.NewRequest("GET", "/greet?name=Ada", nil))
	fmt.Print(rec.Code, " ", rec.Body.String())
}
