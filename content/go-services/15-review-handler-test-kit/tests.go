package main

import (
	"fmt"
	"io"
	"net/http"
	"runtime"
	"strings"
	"testing"
)

type fakeTB struct {
	testing.TB
	helper int
	errs   []string
	fatal  string
}

func (f *fakeTB) Helper()                        { f.helper++ }
func (f *fakeTB) Name() string                   { return "fake" }
func (f *fakeTB) Errorf(format string, a ...any) { f.errs = append(f.errs, fmt.Sprintf(format, a...)) }
func (f *fakeTB) Error(a ...any)                 { f.errs = append(f.errs, fmt.Sprint(a...)) }
func (f *fakeTB) Logf(format string, a ...any)   {}
func (f *fakeTB) Log(a ...any)                   {}
func (f *fakeTB) Fatalf(format string, a ...any) {
	f.fatal = fmt.Sprintf(format, a...)
	runtime.Goexit()
}
func (f *fakeTB) Fatal(a ...any) { f.fatal = fmt.Sprint(a...); runtime.Goexit() }
func (f *fakeTB) run(fn func(testing.TB)) {
	done := make(chan struct{})
	go func() { defer close(done); fn(f) }()
	<-done
}

// echo reports what it received, as JSON.
var echo = http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
	b, _ := io.ReadAll(r.Body)
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(201)
	fmt.Fprintf(w, `{"method":%q,"target":%q,"ctype":%q,"body":%q}`, r.Method, r.URL.RequestURI(), r.Header.Get("Content-Type"), string(b))
})

type echoed struct {
	Method, Target, Ctype, Body string
}

func TestDoJSON_sends_encoded_body(t *testing.T) {
	rec := DoJSON(echo, "POST", "/items?x=1", map[string]any{"name": "pen", "n": 3})
	if rec == nil {
		t.Fatal("DoJSON returned nil")
	}
	if rec.Code != 201 {
		t.Errorf("status %d; the handler's response should come back", rec.Code)
	}
	f := &fakeTB{}
	var got map[string]string
	f.run(func(tb testing.TB) { got = DecodeBody[map[string]string](tb, rec) })
	if f.fatal != "" {
		t.Fatalf("decode failed: %s", f.fatal)
	}
	if got["method"] != "POST" || got["target"] != "/items?x=1" || got["ctype"] != "application/json" {
		t.Errorf("handler saw %v", got)
	}
	if got["body"] != `{"n":3,"name":"pen"}` {
		t.Errorf("body sent = %q", got["body"])
	}
}

func TestDoJSON_struct_body(t *testing.T) {
	type in struct {
		Title string `json:"title"`
	}
	rec := DoJSON(echo, "PUT", "/x", in{"hello"})
	var got map[string]string
	f := &fakeTB{}
	f.run(func(tb testing.TB) { got = DecodeBody[map[string]string](tb, rec) })
	if got["body"] != `{"title":"hello"}` || got["method"] != "PUT" {
		t.Errorf("got %v", got)
	}
}

func TestDoJSON_nil_body_has_no_body_or_content_type(t *testing.T) {
	rec := DoJSON(echo, "GET", "/plain", nil)
	var got map[string]string
	f := &fakeTB{}
	f.run(func(tb testing.TB) { got = DecodeBody[map[string]string](tb, rec) })
	if got["body"] != "" || got["ctype"] != "" {
		t.Errorf("nil body: handler saw body=%q content-type=%q; want both empty", got["body"], got["ctype"])
	}
}

func TestDoJSON_unencodable_body_panics(t *testing.T) {
	defer func() {
		if recover() == nil {
			t.Error("DoJSON should panic when the body cannot be encoded")
		}
	}()
	DoJSON(echo, "POST", "/", make(chan int))
}

func TestDecodeBody_generic_types(t *testing.T) {
	rec := DoJSON(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { fmt.Fprint(w, `[1,2,3]`) }), "GET", "/", nil)
	f := &fakeTB{}
	var got []int
	f.run(func(tb testing.TB) { got = DecodeBody[[]int](tb, rec) })
	if f.fatal != "" || fmt.Sprint(got) != "[1 2 3]" {
		t.Errorf("DecodeBody[[]int] = %v (fatal %q)", got, f.fatal)
	}

	rec = DoJSON(echo, "GET", "/z", nil)
	var e echoed
	f = &fakeTB{}
	f.run(func(tb testing.TB) { e = DecodeBody[echoed](tb, rec) })
	if f.fatal != "" || e.Target != "/z" {
		t.Errorf("DecodeBody[struct] = %+v (fatal %q)", e, f.fatal)
	}
	if f.helper == 0 {
		t.Error("DecodeBody did not call t.Helper()")
	}
}

func TestDecodeBody_bad_json_is_fatal(t *testing.T) {
	rec := DoJSON(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { fmt.Fprint(w, `<html>oops</html>`) }), "GET", "/", nil)
	f := &fakeTB{}
	reached := false
	f.run(func(tb testing.TB) {
		DecodeBody[map[string]string](tb, rec)
		reached = true
	})
	if !strings.HasPrefix(f.fatal, "bad JSON body: ") || !strings.HasSuffix(f.fatal, "\nbody: <html>oops</html>") {
		t.Errorf("fatal = %q", f.fatal)
	}
	if reached {
		t.Error("the test went on after bad JSON; use Fatalf")
	}
	// Wrong shape is also bad JSON for T.
	rec = DoJSON(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { fmt.Fprint(w, `{"a":1}`) }), "GET", "/", nil)
	f = &fakeTB{}
	f.run(func(tb testing.TB) { DecodeBody[[]int](tb, rec) })
	if f.fatal == "" {
		t.Error("decoding an object into []int should be fatal")
	}
}

func TestAssertStatus(t *testing.T) {
	rec := DoJSON(echo, "GET", "/", nil) // 201
	f := &fakeTB{}
	f.run(func(tb testing.TB) { AssertStatus(tb, rec, 201) })
	if len(f.errs) != 0 || f.fatal != "" {
		t.Errorf("matching status reported %v %q", f.errs, f.fatal)
	}
	f = &fakeTB{}
	f.run(func(tb testing.TB) { AssertStatus(tb, rec, 200) })
	if len(f.errs) != 1 || !strings.HasPrefix(f.errs[0], "status = 201, want 200; body: {") {
		t.Errorf("mismatch reported %q", f.errs)
	}
	if f.fatal != "" {
		t.Error("AssertStatus should use Errorf")
	}
	if f.helper == 0 {
		t.Error("AssertStatus did not call t.Helper()")
	}
}

func errHandler(code int, body string) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(code)
		fmt.Fprint(w, body)
	})
}

func TestAssertJSONError_ok(t *testing.T) {
	rec := DoJSON(errHandler(400, `{"error":"title required"}`), "POST", "/", nil)
	f := &fakeTB{}
	f.run(func(tb testing.TB) { AssertJSONError(tb, rec, 400, "title required") })
	if len(f.errs) != 0 || f.fatal != "" {
		t.Errorf("correct error reported %v %q", f.errs, f.fatal)
	}
}

func TestAssertJSONError_wrong_message_or_status(t *testing.T) {
	rec := DoJSON(errHandler(404, `{"error":"not found"}`), "GET", "/", nil)
	f := &fakeTB{}
	f.run(func(tb testing.TB) { AssertJSONError(tb, rec, 404, "gone") })
	if len(f.errs) != 1 || f.errs[0] != `error = "not found", want "gone"` {
		t.Errorf("wrong message reported %q", f.errs)
	}
	f = &fakeTB{}
	f.run(func(tb testing.TB) { AssertJSONError(tb, rec, 400, "not found") })
	if len(f.errs) != 1 || !strings.HasPrefix(f.errs[0], "status = 404, want 400") {
		t.Errorf("wrong status reported %q", f.errs)
	}
	if f.helper == 0 {
		t.Error("AssertJSONError did not call t.Helper()")
	}
}

func TestAssertJSONError_non_json_body_is_fatal(t *testing.T) {
	rec := DoJSON(errHandler(500, `boom`), "GET", "/", nil)
	f := &fakeTB{}
	f.run(func(tb testing.TB) { AssertJSONError(tb, rec, 500, "boom") })
	if !strings.HasPrefix(f.fatal, "bad JSON body: ") {
		t.Errorf("fatal = %q; a non-JSON body should be fatal", f.fatal)
	}
}
