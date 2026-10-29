package main

import (
	"encoding/json"
	"fmt"
	"net/http"
	"net/http/httptest"
	"strconv"
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

func writeJSON(w http.ResponseWriter, status int, v any) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(status)
	json.NewEncoder(w).Encode(v)
}

func writeError(w http.ResponseWriter, status int, msg string) {
	writeJSON(w, status, map[string]string{"error": msg})
}

func (s *Store) create(w http.ResponseWriter, r *http.Request) {
	dec := json.NewDecoder(r.Body)
	dec.DisallowUnknownFields()
	var in struct {
		Title string `json:"title"`
	}
	if err := dec.Decode(&in); err != nil {
		writeError(w, http.StatusBadRequest, "invalid json")
		return
	}
	if strings.TrimSpace(in.Title) == "" {
		writeError(w, http.StatusBadRequest, "title required")
		return
	}
	s.mu.Lock()
	t := Todo{ID: len(s.todos) + 1, Title: in.Title}
	s.todos = append(s.todos, t)
	s.mu.Unlock()
	w.Header().Set("Location", "/todos/"+strconv.Itoa(t.ID))
	writeJSON(w, http.StatusCreated, t)
}

func (s *Store) list(w http.ResponseWriter, r *http.Request) {
	s.mu.Lock()
	out := make([]Todo, len(s.todos))
	copy(out, s.todos)
	s.mu.Unlock()
	writeJSON(w, http.StatusOK, out)
}

func (s *Store) get(w http.ResponseWriter, r *http.Request) {
	s.withTodo(w, r, false)
}

func (s *Store) done(w http.ResponseWriter, r *http.Request) {
	s.withTodo(w, r, true)
}

func (s *Store) withTodo(w http.ResponseWriter, r *http.Request, markDone bool) {
	id, err := strconv.Atoi(r.PathValue("id"))
	s.mu.Lock()
	if err != nil || id < 1 || id > len(s.todos) {
		s.mu.Unlock()
		writeError(w, http.StatusNotFound, "not found")
		return
	}
	if markDone {
		s.todos[id-1].Done = true
	}
	t := s.todos[id-1]
	s.mu.Unlock()
	writeJSON(w, http.StatusOK, t)
}

func (s *Store) Handler() http.Handler {
	mux := http.NewServeMux()
	mux.HandleFunc("POST /todos", s.create)
	mux.HandleFunc("GET /todos", s.list)
	mux.HandleFunc("GET /todos/{id}", s.get)
	mux.HandleFunc("POST /todos/{id}/done", s.done)
	return mux
}

func main() {
	h := NewStore().Handler()
	rec := httptest.NewRecorder()
	h.ServeHTTP(rec, httptest.NewRequest("POST", "/todos", strings.NewReader(`{"title":"learn Go"}`)))
	fmt.Print(rec.Code, " ", rec.Body.String())
}
