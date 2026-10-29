package main

import (
	"context"
	"errors"
	"fmt"
	"net/http"
	"net/http/httptest"
	"testing"
	"time"
)

func get(h http.Handler, ctx context.Context, target string, hdr ...string) *httptest.ResponseRecorder {
	req := httptest.NewRequestWithContext(ctx, "GET", target, nil)
	for i := 0; i+1 < len(hdr); i += 2 {
		req.Header.Set(hdr[i], hdr[i+1])
	}
	rec := httptest.NewRecorder()
	h.ServeHTTP(rec, req)
	return rec
}

func TestLookup_success_and_missing_key(t *testing.T) {
	h := Lookup(func(ctx context.Context, key string) (string, error) { return "value-of-" + key, nil })
	for _, k := range []string{"a", "xyz"} {
		rec := get(h, context.Background(), "/?key="+k)
		if rec.Code != 200 || rec.Body.String() != "value-of-"+k {
			t.Errorf("key %s: %d %q", k, rec.Code, rec.Body.String())
		}
	}
	called := false
	h2 := Lookup(func(ctx context.Context, key string) (string, error) { called = true; return "", nil })
	for _, target := range []string{"/", "/?key="} {
		rec := get(h2, context.Background(), target)
		if rec.Code != 400 {
			t.Errorf("%s: status %d; want 400", target, rec.Code)
		}
	}
	if called {
		t.Error("find was called without a key")
	}
}

func TestLookup_maps_errors_to_status(t *testing.T) {
	cases := []struct {
		err  error
		code int
	}{
		{ErrNotFound, 404},
		{fmt.Errorf("db: %w", ErrNotFound), 404},
		{context.DeadlineExceeded, 504},
		{fmt.Errorf("slow: %w", context.DeadlineExceeded), 504},
		{context.Canceled, 499},
		{fmt.Errorf("gone: %w", context.Canceled), 499},
		{errors.New("not found"), 500}, // same text, different error: check types, not strings
		{errors.New("disk on fire"), 500},
	}
	for _, c := range cases {
		h := Lookup(func(ctx context.Context, key string) (string, error) { return "", c.err })
		if rec := get(h, context.Background(), "/?key=k"); rec.Code != c.code {
			t.Errorf("error %q: status %d; want %d", c.err, rec.Code, c.code)
		}
	}
}

func TestLookup_passes_the_request_context(t *testing.T) {
	type k struct{}
	ctx, cancel := context.WithCancel(context.WithValue(context.Background(), k{}, "marker"))
	cancel()
	var seen context.Context
	h := Lookup(func(c context.Context, key string) (string, error) {
		seen = c
		return "", c.Err()
	})
	rec := get(h, ctx, "/?key=a")
	if seen == nil || seen.Value(k{}) != "marker" {
		t.Fatal("find did not receive the request's context (use r.Context())")
	}
	if rec.Code != 499 {
		t.Errorf("cancelled request: status %d; want 499", rec.Code)
	}
}

func TestTimeout_gives_handler_a_deadline(t *testing.T) {
	var dl time.Time
	var ok bool
	h := Timeout(time.Hour)(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		dl, ok = r.Context().Deadline()
	}))
	get(h, context.Background(), "/")
	if !ok {
		t.Fatal("handler's context has no deadline")
	}
	if left := time.Until(dl); left < 59*time.Minute || left > time.Hour {
		t.Errorf("deadline is %v away; want about 1h", left)
	}
}

func TestTimeout_expires_and_lookup_answers_504(t *testing.T) {
	blocking := Lookup(func(ctx context.Context, key string) (string, error) {
		<-ctx.Done() // only the middleware's timeout can release this
		return "", ctx.Err()
	})
	rec := get(Timeout(5*time.Millisecond)(blocking), context.Background(), "/?key=a")
	if rec.Code != 504 {
		t.Errorf("status %d; want 504", rec.Code)
	}
}

func TestTimeout_keeps_parent_cancellation(t *testing.T) {
	parent, cancel := context.WithCancel(context.Background())
	cancel()
	var err error
	h := Timeout(time.Hour)(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { err = r.Context().Err() }))
	get(h, parent, "/")
	if !errors.Is(err, context.Canceled) {
		t.Errorf("handler context Err = %v; it must derive from the request's context", err)
	}
}

func TestRequestID(t *testing.T) {
	n := 0
	gen := func() string { n++; return fmt.Sprintf("gen-%d", n) }
	var seen string
	h := RequestID(gen)(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		seen = IDFrom(r.Context())
	}))

	rec := get(h, context.Background(), "/")
	if seen != "gen-1" || rec.Header().Get("X-Request-ID") != "gen-1" {
		t.Errorf("generated: ctx=%q header=%q", seen, rec.Header().Get("X-Request-ID"))
	}
	rec = get(h, context.Background(), "/", "X-Request-ID", "client-77")
	if seen != "client-77" || rec.Header().Get("X-Request-ID") != "client-77" {
		t.Errorf("supplied: ctx=%q header=%q", seen, rec.Header().Get("X-Request-ID"))
	}
	if n != 1 {
		t.Errorf("gen was called %d times; only when the header is empty", n)
	}
}

func TestRequestID_header_set_even_if_handler_writes(t *testing.T) {
	h := RequestID(func() string { return "g" })(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		fmt.Fprint(w, "body")
	}))
	rec := get(h, context.Background(), "/")
	if got := rec.Result().Header.Get("X-Request-ID"); got != "g" {
		t.Errorf("sent X-Request-ID = %q; set it before calling next", got)
	}
}

func TestIDFrom_empty(t *testing.T) {
	if got := IDFrom(context.Background()); got != "" {
		t.Errorf("IDFrom(empty) = %q", got)
	}
	// A string key from elsewhere must not be mistaken for the ID.
	ctx := context.WithValue(context.Background(), "request_id", "other")
	if got := IDFrom(ctx); got != "" {
		t.Errorf("IDFrom picked up a plain string key: %q", got)
	}
}
