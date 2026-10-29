package main

import (
	"fmt"
	"net/http"
	"net/http/httptest"
	"strings"
	"sync"
)

type Todo struct {
	ID    int    `json:"id"`
	Title string `json:"title"`
	Done  bool   `json:"done"`
}

type Store struct {
	mu    sync.Mutex
	todos []Todo
}

func NewStore() *Store {
	return &Store{}
}

func (s *Store) Handler() http.Handler {
	return http.NewServeMux()
}

// Try it with Run.
func main() {
	h := NewStore().Handler()
	rec := httptest.NewRecorder()
	h.ServeHTTP(rec, httptest.NewRequest("POST", "/todos", strings.NewReader(`{"title":"learn Go"}`)))
	fmt.Print(rec.Code, " ", rec.Body.String())
}
