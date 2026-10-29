package main

import (
	"fmt"
	"io"
	"net/http"
	"net/http/httptest"
	"testing"
)

type entry struct {
	method, path  string
	status, bytes int
}

func logged(t *testing.T, h http.Handler, method, path string) (entry, *httptest.ResponseRecorder) {
	t.Helper()
	var got []entry
	wrapped := AccessLog(func(m, p string, s, b int) { got = append(got, entry{m, p, s, b}) })(h)
	rec := httptest.NewRecorder()
	wrapped.ServeHTTP(rec, httptest.NewRequest(method, path, nil))
	if len(got) != 1 {
		t.Fatalf("log was called %d times; want exactly 1", len(got))
	}
	return got[0], rec
}

func TestAccessLog_explicit_status(t *testing.T) {
	h := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusTeapot)
		io.WriteString(w, "short and stout")
	})
	e, rec := logged(t, h, "POST", "/tea")
	if e != (entry{"POST", "/tea", 418, 15}) {
		t.Errorf("entry = %+v", e)
	}
	if rec.Code != 418 || rec.Body.String() != "short and stout" {
		t.Errorf("response changed: %d %q", rec.Code, rec.Body.String())
	}
}

func TestAccessLog_implicit_200_and_byte_count(t *testing.T) {
	h := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		fmt.Fprint(w, "abc")
		fmt.Fprint(w, "defgh")
	})
	e, _ := logged(t, h, "GET", "/x?q=1")
	if e != (entry{"GET", "/x", 200, 8}) {
		t.Errorf("entry = %+v; want GET /x 200 8 (path without the query)", e)
	}
}

func TestAccessLog_nothing_written_is_200(t *testing.T) {
	e, _ := logged(t, http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {}), "GET", "/")
	if e.status != 200 || e.bytes != 0 {
		t.Errorf("entry = %+v; want status 200, 0 bytes", e)
	}
}

func TestAccessLog_only_first_status_counts(t *testing.T) {
	h := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(404)
		w.WriteHeader(500) // ignored by net/http, so it must be ignored by the log too
	})
	e, _ := logged(t, h, "GET", "/")
	if e.status != 404 {
		t.Errorf("status = %d; want the first one, 404", e.status)
	}
}

func TestStatusWriter_unwrap_exposes_flusher(t *testing.T) {
	flushed := false
	h := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		io.WriteString(w, "x")
		if err := http.NewResponseController(w).Flush(); err == nil {
			flushed = true
		}
	})
	logged(t, h, "GET", "/")
	if !flushed {
		t.Error("http.NewResponseController(w).Flush() failed through your wrapper; give statusWriter an Unwrap method that returns the wrapped writer")
	}
}

func TestRecover_turns_panic_into_500(t *testing.T) {
	for _, val := range []any{"oops", fmt.Errorf("bad thing"), 42} {
		var reported []any
		h := Recover(func(v any) { reported = append(reported, v) })(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { panic(val) }))
		rec := httptest.NewRecorder()
		h.ServeHTTP(rec, httptest.NewRequest("GET", "/", nil))
		if rec.Code != 500 || rec.Body.String() != "internal server error\n" {
			t.Errorf("panic(%v): %d %q; want 500 \"internal server error\\n\"", val, rec.Code, rec.Body.String())
		}
		if len(reported) != 1 || fmt.Sprint(reported[0]) != fmt.Sprint(val) {
			t.Errorf("panic(%v): reported %v", val, reported)
		}
	}
}

func TestRecover_passes_normal_requests(t *testing.T) {
	h := Recover(func(v any) { t.Errorf("nothing should be reported, got %v", v) })(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(201)
		io.WriteString(w, "made")
	}))
	rec := httptest.NewRecorder()
	h.ServeHTTP(rec, httptest.NewRequest("GET", "/", nil))
	if rec.Code != 201 || rec.Body.String() != "made" {
		t.Errorf("got %d %q", rec.Code, rec.Body.String())
	}
}

func TestRecover_after_partial_response_keeps_status(t *testing.T) {
	reported := 0
	h := Recover(func(v any) { reported++ })(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(202)
		io.WriteString(w, "half")
		panic("late")
	}))
	rec := httptest.NewRecorder()
	h.ServeHTTP(rec, httptest.NewRequest("GET", "/", nil))
	if rec.Code != 202 || rec.Body.String() != "half" {
		t.Errorf("got %d %q; a started response must be left alone", rec.Code, rec.Body.String())
	}
	if reported != 1 {
		t.Errorf("reported %d times; want 1", reported)
	}
}

func TestRecover_abort_handler_is_repanicked(t *testing.T) {
	defer func() {
		if v := recover(); v != http.ErrAbortHandler {
			t.Errorf("recovered %v; want http.ErrAbortHandler to escape", v)
		}
	}()
	h := Recover(func(v any) { t.Error("ErrAbortHandler must not be reported") })(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		panic(http.ErrAbortHandler)
	}))
	h.ServeHTTP(httptest.NewRecorder(), httptest.NewRequest("GET", "/", nil))
	t.Error("the panic did not escape")
}

func TestChain_log_shows_500_for_panicking_handler(t *testing.T) {
	var got []entry
	var h http.Handler = http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { panic("boom") })
	h = Recover(func(v any) {})(h)
	h = AccessLog(func(m, p string, s, b int) { got = append(got, entry{m, p, s, b}) })(h)
	rec := httptest.NewRecorder()
	h.ServeHTTP(rec, httptest.NewRequest("GET", "/boom", nil))
	if len(got) != 1 || got[0].status != 500 || got[0].bytes != len("internal server error\n") {
		t.Errorf("log = %+v; want one entry with status 500 and %d bytes", got, len("internal server error\n"))
	}
}
