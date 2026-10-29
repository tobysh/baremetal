package main

import (
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"reflect"
	"strconv"
	"strings"
	"sync"
	"testing"
)

func send(h http.Handler, method, target, body string) *httptest.ResponseRecorder {
	rec := httptest.NewRecorder()
	var r *http.Request
	if body == "" {
		r = httptest.NewRequest(method, target, nil)
	} else {
		r = httptest.NewRequest(method, target, strings.NewReader(body))
	}
	h.ServeHTTP(rec, r)
	return rec
}

func mustJSON(t *testing.T, rec *httptest.ResponseRecorder, v any) {
	t.Helper()
	if ct := rec.Header().Get("Content-Type"); ct != "application/json" {
		t.Errorf("Content-Type = %q; want application/json", ct)
	}
	if err := json.Unmarshal(rec.Body.Bytes(), v); err != nil {
		t.Fatalf("response body is not valid JSON: %v (%q)", err, rec.Body.String())
	}
}

func TestCreate(t *testing.T) {
	h := NewStore().Handler()
	for i, title := range []string{"write tests", "ship it", "  padded  "} {
		rec := send(h, "POST", "/todos", `{"title":`+strconv.Quote(title)+`}`)
		if rec.Code != 201 {
			t.Fatalf("POST status %d; want 201 (%s)", rec.Code, rec.Body.String())
		}
		var got Todo
		mustJSON(t, rec, &got)
		want := Todo{ID: i + 1, Title: title}
		if got != want {
			t.Errorf("created %+v; want %+v", got, want)
		}
		if loc := rec.Header().Get("Location"); loc != "/todos/"+strconv.Itoa(i+1) {
			t.Errorf("Location = %q", loc)
		}
	}
}

func TestCreate_rejects_bad_input(t *testing.T) {
	cases := []struct{ body, msg string }{
		{`{"title":`, "invalid json"},
		{`nonsense`, "invalid json"},
		{``, "invalid json"},
		{`{"title":"x","extra":1}`, "invalid json"},
		{`{"title":5}`, "invalid json"},
		{`{}`, "title required"},
		{`{"title":""}`, "title required"},
		{`{"title":"   "}`, "title required"},
	}
	for _, c := range cases {
		s := NewStore()
		h := s.Handler()
		// An empty body must reach the handler as a body, so use a non-nil reader.
		rec := httptest.NewRecorder()
		h.ServeHTTP(rec, httptest.NewRequest("POST", "/todos", strings.NewReader(c.body)))
		if rec.Code != 400 {
			t.Errorf("body %q: status %d; want 400", c.body, rec.Code)
			continue
		}
		var e map[string]string
		mustJSON(t, rec, &e)
		if e["error"] != c.msg {
			t.Errorf("body %q: error %q; want %q", c.body, e["error"], c.msg)
		}
		var list []Todo
		mustJSON(t, send(h, "GET", "/todos", ""), &list)
		if len(list) != 0 {
			t.Errorf("body %q: a rejected request created %v", c.body, list)
		}
	}
}

func TestList_empty_is_array_and_keeps_order(t *testing.T) {
	h := NewStore().Handler()
	rec := send(h, "GET", "/todos", "")
	if rec.Code != 200 || strings.TrimSpace(rec.Body.String()) != "[]" {
		t.Errorf("empty list = %d %q; want 200 []", rec.Code, rec.Body.String())
	}
	for _, title := range []string{"a", "b", "c"} {
		send(h, "POST", "/todos", `{"title":"`+title+`"}`)
	}
	var got []Todo
	mustJSON(t, send(h, "GET", "/todos", ""), &got)
	want := []Todo{{1, "a", false}, {2, "b", false}, {3, "c", false}}
	if !reflect.DeepEqual(got, want) {
		t.Errorf("list = %+v; want %+v", got, want)
	}
}

func TestGetAndDone(t *testing.T) {
	h := NewStore().Handler()
	send(h, "POST", "/todos", `{"title":"one"}`)
	send(h, "POST", "/todos", `{"title":"two"}`)

	var got Todo
	mustJSON(t, send(h, "GET", "/todos/2", ""), &got)
	if got != (Todo{2, "two", false}) {
		t.Errorf("GET /todos/2 = %+v", got)
	}
	rec := send(h, "POST", "/todos/2/done", "")
	if rec.Code != 200 {
		t.Fatalf("POST done = %d", rec.Code)
	}
	mustJSON(t, rec, &got)
	if got != (Todo{2, "two", true}) {
		t.Errorf("after done = %+v", got)
	}
	mustJSON(t, send(h, "GET", "/todos/2", ""), &got)
	if !got.Done {
		t.Error("done flag was not stored")
	}
	mustJSON(t, send(h, "GET", "/todos/1", ""), &got)
	if got.Done {
		t.Error("marking todo 2 done also marked todo 1")
	}
}

func TestNotFound_has_json_error(t *testing.T) {
	h := NewStore().Handler()
	send(h, "POST", "/todos", `{"title":"only"}`)
	for _, c := range [][2]string{{"GET", "/todos/2"}, {"GET", "/todos/0"}, {"GET", "/todos/-1"}, {"GET", "/todos/abc"}, {"POST", "/todos/9/done"}, {"POST", "/todos/x/done"}} {
		rec := send(h, c[0], c[1], "")
		if rec.Code != 404 {
			t.Errorf("%s %s = %d; want 404", c[0], c[1], rec.Code)
			continue
		}
		var e map[string]string
		mustJSON(t, rec, &e)
		if e["error"] != "not found" {
			t.Errorf("%s %s error = %q", c[0], c[1], e["error"])
		}
	}
}

func TestConcurrent_creates_get_unique_ids(t *testing.T) {
	h := NewStore().Handler()
	const n = 50
	var wg sync.WaitGroup
	for i := 0; i < n; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			send(h, "POST", "/todos", `{"title":"x"}`)
		}()
	}
	wg.Wait()
	var list []Todo
	mustJSON(t, send(h, "GET", "/todos", ""), &list)
	if len(list) != n {
		t.Fatalf("%d todos after %d creates", len(list), n)
	}
	for i, td := range list {
		if td.ID != i+1 {
			t.Fatalf("todo at position %d has id %d; ids must be unique and increasing", i, td.ID)
		}
	}
}
