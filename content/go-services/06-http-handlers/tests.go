package main

import (
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"
)

func do(h http.Handler, method, target string) *httptest.ResponseRecorder {
	rec := httptest.NewRecorder()
	h.ServeHTTP(rec, httptest.NewRequest(method, target, nil))
	return rec
}

func TestGreet(t *testing.T) {
	cases := []struct{ target, want string }{
		{"/greet?name=Ada", "Hello, Ada!\n"},
		{"/greet?name=Grace+Hopper", "Hello, Grace Hopper!\n"},
		{"/greet", "Hello, world!\n"},
		{"/greet?name=", "Hello, world!\n"},
	}
	for _, c := range cases {
		rec := do(http.HandlerFunc(Greet), "GET", c.target)
		if rec.Code != 200 || rec.Body.String() != c.want {
			t.Errorf("Greet %s = %d %q; want 200 %q", c.target, rec.Code, rec.Body.String(), c.want)
		}
		if ct := rec.Header().Get("Content-Type"); ct != "text/plain; charset=utf-8" {
			t.Errorf("Greet %s Content-Type = %q", c.target, ct)
		}
	}
}

func TestGreet_wrong_method(t *testing.T) {
	for _, m := range []string{"POST", "PUT", "DELETE"} {
		rec := do(http.HandlerFunc(Greet), m, "/greet?name=x")
		if rec.Code != http.StatusMethodNotAllowed {
			t.Errorf("%s: status %d; want 405", m, rec.Code)
		}
		if got := rec.Header().Get("Allow"); got != "GET" {
			t.Errorf("%s: Allow = %q; want GET", m, got)
		}
	}
}

func TestMux_routes(t *testing.T) {
	mux := NewMux()
	cases := []struct {
		method, target string
		code           int
		body           string
	}{
		{"GET", "/greet?name=Bo", 200, "Hello, Bo!\n"},
		{"GET", "/health", 200, "ok"},
		{"GET", "/users/7", 200, "user 7"},
		{"GET", "/users/12345", 200, "user 12345"},
		{"GET", "/users/abc", 400, ""},
		{"GET", "/users/-5", 400, ""},
		{"GET", "/users/+5", 400, ""},
		{"GET", "/users/1x", 400, ""},
	}
	for _, c := range cases {
		rec := do(mux, c.method, c.target)
		if rec.Code != c.code {
			t.Errorf("%s %s = %d; want %d", c.method, c.target, rec.Code, c.code)
		}
		if c.code == 200 && rec.Body.String() != c.body {
			t.Errorf("%s %s body = %q; want %q", c.method, c.target, rec.Body.String(), c.body)
		}
	}
}

func TestMux_404_and_405(t *testing.T) {
	mux := NewMux()
	if rec := do(mux, "GET", "/nothing"); rec.Code != 404 {
		t.Errorf("GET /nothing = %d; want 404", rec.Code)
	}
	if rec := do(mux, "GET", "/users/"); rec.Code != 404 {
		t.Errorf("GET /users/ = %d; want 404", rec.Code)
	}
	for _, p := range []string{"/health", "/greet", "/users/3"} {
		rec := do(mux, "POST", p)
		if rec.Code != http.StatusMethodNotAllowed {
			t.Errorf("POST %s = %d; want 405 (register routes with a method, e.g. \"GET /health\")", p, rec.Code)
		}
		if !strings.Contains(rec.Header().Get("Allow"), "GET") {
			t.Errorf("POST %s: Allow = %q; want it to list GET", p, rec.Header().Get("Allow"))
		}
	}
}
