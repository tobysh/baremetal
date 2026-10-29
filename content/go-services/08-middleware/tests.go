package main

import (
	"errors"
	"io"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"
)

var ok200 = http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { io.WriteString(w, "fine") })

func TestChain_order(t *testing.T) {
	var log []string
	mark := func(name string) Middleware {
		return func(next http.Handler) http.Handler {
			return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				log = append(log, name+" in")
				next.ServeHTTP(w, r)
				log = append(log, name+" out")
			})
		}
	}
	h := Chain(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { log = append(log, "handler") }), mark("A"), mark("B"), mark("C"))
	h.ServeHTTP(httptest.NewRecorder(), httptest.NewRequest("GET", "/", nil))
	want := "A in,B in,C in,handler,C out,B out,A out"
	if got := strings.Join(log, ","); got != want {
		t.Errorf("order = %s\nwant    %s", got, want)
	}
}

func TestChain_empty(t *testing.T) {
	rec := httptest.NewRecorder()
	Chain(ok200).ServeHTTP(rec, httptest.NewRequest("GET", "/", nil))
	if rec.Body.String() != "fine" {
		t.Errorf("Chain with no middleware broke the handler: %q", rec.Body.String())
	}
}

func TestRequireToken(t *testing.T) {
	cases := []struct {
		token, header string
		code          int
	}{
		{"s3cret", "Bearer s3cret", 200},
		{"s3cret", "", 401},
		{"s3cret", "Bearer wrong", 401},
		{"s3cret", "s3cret", 401},
		{"s3cret", "Bearer s3cret ", 401},
		{"s3cret", "bearer s3cret", 401},
		{"other", "Bearer other", 200},
		{"other", "Bearer s3cret", 401},
	}
	for _, c := range cases {
		called := false
		h := Chain(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { called = true }), RequireToken(c.token))
		req := httptest.NewRequest("GET", "/", nil)
		if c.header != "" {
			req.Header.Set("Authorization", c.header)
		}
		rec := httptest.NewRecorder()
		h.ServeHTTP(rec, req)
		if rec.Code != c.code {
			t.Errorf("token %q header %q: status %d; want %d", c.token, c.header, rec.Code, c.code)
		}
		if called != (c.code == 200) {
			t.Errorf("token %q header %q: handler called = %v", c.token, c.header, called)
		}
		if c.code == 401 && rec.Header().Get("WWW-Authenticate") != "Bearer" {
			t.Errorf("token %q header %q: missing WWW-Authenticate: Bearer", c.token, c.header)
		}
	}
}

func TestSetHeader(t *testing.T) {
	h := Chain(ok200, SetHeader("X-Frame-Options", "DENY"), SetHeader("X-Version", "7"))
	rec := httptest.NewRecorder()
	h.ServeHTTP(rec, httptest.NewRequest("GET", "/", nil))
	if rec.Header().Get("X-Frame-Options") != "DENY" || rec.Header().Get("X-Version") != "7" {
		t.Errorf("headers = %v", rec.Header())
	}
	if rec.Body.String() != "fine" {
		t.Errorf("body = %q; the handler must still run", rec.Body.String())
	}
}

func TestSetHeader_can_be_overridden_by_handler(t *testing.T) {
	inner := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { w.Header().Set("X-Mode", "inner") })
	rec := httptest.NewRecorder()
	Chain(inner, SetHeader("X-Mode", "outer")).ServeHTTP(rec, httptest.NewRequest("GET", "/", nil))
	if got := rec.Header().Get("X-Mode"); got != "inner" {
		t.Errorf("X-Mode = %q; SetHeader must run before the handler so the handler can override it", got)
	}
}

func TestSetHeader_visible_to_handler_that_writes(t *testing.T) {
	// The header must be set before the handler writes, or it never reaches the client.
	writer := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { io.WriteString(w, "body") })
	rec := httptest.NewRecorder()
	Chain(writer, SetHeader("X-Early", "yes")).ServeHTTP(rec, httptest.NewRequest("GET", "/", nil))
	if got := rec.Result().Header.Get("X-Early"); got != "yes" {
		t.Errorf("X-Early in the sent response = %q; set it before calling next", got)
	}
}

func TestMaxBody(t *testing.T) {
	reader := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		b, err := io.ReadAll(r.Body)
		var tooBig *http.MaxBytesError
		if errors.As(err, &tooBig) {
			http.Error(w, "too big", http.StatusRequestEntityTooLarge)
			return
		}
		if err != nil {
			http.Error(w, err.Error(), 500)
			return
		}
		w.Write(b)
	})
	cases := []struct {
		limit int64
		body  string
		code  int
	}{
		{10, "short", 200},
		{10, "0123456789", 200},
		{10, "0123456789A", 413},
		{0, "x", 413},
		{0, "", 200},
	}
	for _, c := range cases {
		rec := httptest.NewRecorder()
		Chain(reader, MaxBody(c.limit)).ServeHTTP(rec, httptest.NewRequest("POST", "/", strings.NewReader(c.body)))
		if rec.Code != c.code {
			t.Errorf("limit %d body %q: status %d; want %d", c.limit, c.body, rec.Code, c.code)
		}
		if c.code == 200 && rec.Body.String() != c.body {
			t.Errorf("limit %d: echoed %q; want %q", c.limit, rec.Body.String(), c.body)
		}
	}
}
