package main

import (
	"fmt"
	"net/http"
	"net/http/httptest"
)

func Greet(w http.ResponseWriter, r *http.Request) {
}

func NewMux() *http.ServeMux {
	mux := http.NewServeMux()
	return mux
}

// Try it with Run: call the router without any network.
func main() {
	rec := httptest.NewRecorder()
	NewMux().ServeHTTP(rec, httptest.NewRequest("GET", "/greet?name=Ada", nil))
	fmt.Print(rec.Code, " ", rec.Body.String())
}
