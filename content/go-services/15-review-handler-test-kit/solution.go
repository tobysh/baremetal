package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"net/http/httptest"
	"testing"
)

func DoJSON(h http.Handler, method, target string, body any) *httptest.ResponseRecorder {
	var r io.Reader
	if body != nil {
		b, err := json.Marshal(body)
		if err != nil {
			panic(err)
		}
		r = bytes.NewReader(b)
	}
	req := httptest.NewRequest(method, target, r)
	if body != nil {
		req.Header.Set("Content-Type", "application/json")
	}
	rec := httptest.NewRecorder()
	h.ServeHTTP(rec, req)
	return rec
}

func DecodeBody[T any](t testing.TB, rec *httptest.ResponseRecorder) T {
	t.Helper()
	var v T
	if err := json.Unmarshal(rec.Body.Bytes(), &v); err != nil {
		t.Fatalf("bad JSON body: %v\nbody: %s", err, rec.Body.String())
		var zero T
		return zero
	}
	return v
}

func AssertStatus(t testing.TB, rec *httptest.ResponseRecorder, want int) {
	t.Helper()
	if rec.Code != want {
		t.Errorf("status = %d, want %d; body: %s", rec.Code, want, rec.Body.String())
	}
}

func AssertJSONError(t testing.TB, rec *httptest.ResponseRecorder, status int, msg string) {
	t.Helper()
	AssertStatus(t, rec, status)
	var e struct {
		Error string `json:"error"`
	}
	if err := json.Unmarshal(rec.Body.Bytes(), &e); err != nil {
		t.Fatalf("bad JSON body: %v\nbody: %s", err, rec.Body.String())
		return
	}
	if e.Error != msg {
		t.Errorf("error = %q, want %q", e.Error, msg)
	}
}

func main() {
	h := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { fmt.Fprint(w, `{"ok":true}`) })
	rec := DoJSON(h, "GET", "/", nil)
	fmt.Println(rec != nil)
}
