package main

import (
	"context"
	"errors"
	"io"
	"net/http"
	"strings"
	"testing"
	"time"
)

type trackedBody struct {
	io.Reader
	closed *bool
}

func (b trackedBody) Close() error { *b.closed = true; return nil }

func reply(status int, body string, closed *bool) *http.Response {
	var rc io.ReadCloser = io.NopCloser(strings.NewReader(body))
	if closed != nil {
		rc = trackedBody{strings.NewReader(body), closed}
	}
	return &http.Response{StatusCode: status, Status: http.StatusText(status), Header: http.Header{}, Body: rc}
}

type item struct {
	N    int    `json:"n"`
	Name string `json:"name"`
}

func TestNewTestClient_uses_the_function(t *testing.T) {
	var gotURL string
	c := NewTestClient(func(r *http.Request) (*http.Response, error) {
		gotURL = r.URL.String()
		return reply(200, "{}", nil), nil
	})
	resp, err := c.Get("http://svc.test/ping")
	if err != nil {
		t.Fatal(err)
	}
	resp.Body.Close()
	if gotURL != "http://svc.test/ping" {
		t.Errorf("transport saw %q", gotURL)
	}
}

func TestFetchJSON_decodes_and_sets_headers(t *testing.T) {
	var method, accept string
	c := NewTestClient(func(r *http.Request) (*http.Response, error) {
		method, accept = r.Method, r.Header.Get("Accept")
		return reply(200, `{"n":7,"name":"seven","extra":true}`, nil), nil
	})
	var got item
	if err := FetchJSON(context.Background(), c, "http://svc.test/x", &got); err != nil {
		t.Fatalf("FetchJSON: %v", err)
	}
	if got != (item{7, "seven"}) {
		t.Errorf("decoded %+v", got)
	}
	if method != "GET" || accept != "application/json" {
		t.Errorf("method=%q accept=%q", method, accept)
	}
}

func TestFetchJSON_closes_body(t *testing.T) {
	for _, tc := range []struct {
		status int
		body   string
	}{{200, `{"n":1}`}, {200, `not json`}, {500, "oops"}} {
		closed := false
		c := NewTestClient(func(r *http.Request) (*http.Response, error) { return reply(tc.status, tc.body, &closed), nil })
		var v item
		FetchJSON(context.Background(), c, "http://svc.test/", &v)
		if !closed {
			t.Errorf("status %d body %q: the response body was not closed", tc.status, tc.body)
		}
	}
}

func TestFetchJSON_bad_status_is_httperror(t *testing.T) {
	for _, st := range []int{201, 404, 500, 301} {
		c := NewTestClient(func(r *http.Request) (*http.Response, error) { return reply(st, `{"n":5}`, nil), nil })
		var v item
		err := FetchJSON(context.Background(), c, "http://svc.test/", &v)
		var he *HTTPError
		if st == 201 {
			if err != nil || v.N != 5 {
				t.Errorf("201 is a success: err=%v v=%+v", err, v)
			}
			continue
		}
		if !errors.As(err, &he) || he.Status != st {
			t.Errorf("status %d: err = %v; want *HTTPError with that status", st, err)
		}
	}
}

func TestHTTPError_text_and_body_cap(t *testing.T) {
	long := strings.Repeat("x", 500)
	c := NewTestClient(func(r *http.Request) (*http.Response, error) { return reply(404, long, nil), nil })
	err := FetchJSON(context.Background(), c, "http://svc.test/", nil)
	var he *HTTPError
	if !errors.As(err, &he) {
		t.Fatalf("err = %v", err)
	}
	if len(he.Body) != 100 {
		t.Errorf("HTTPError.Body has %d bytes; want the first 100", len(he.Body))
	}
	if (&HTTPError{Status: 418, Body: "teapot"}).Error() != "http 418: teapot" {
		t.Errorf("Error() = %q", (&HTTPError{Status: 418, Body: "teapot"}).Error())
	}
}

func TestFetchJSON_invalid_json(t *testing.T) {
	c := NewTestClient(func(r *http.Request) (*http.Response, error) { return reply(200, `{"n":`, nil), nil })
	var v item
	if err := FetchJSON(context.Background(), c, "http://svc.test/", &v); err == nil {
		t.Error("truncated JSON returned no error")
	}
}

func TestFetchJSON_transport_error_passes_through(t *testing.T) {
	boom := errors.New("connection refused")
	c := NewTestClient(func(r *http.Request) (*http.Response, error) { return nil, boom })
	err := FetchJSON(context.Background(), c, "http://svc.test/", nil)
	if !errors.Is(err, boom) {
		t.Errorf("err = %v; want it to wrap the transport error", err)
	}
}

func TestFetchJSON_honours_context(t *testing.T) {
	entered := make(chan struct{})
	c := NewTestClient(func(r *http.Request) (*http.Response, error) {
		close(entered)
		<-r.Context().Done() // a "server" that never answers
		return nil, r.Context().Err()
	})
	ctx, cancel := context.WithCancel(context.Background())
	res := make(chan error, 1)
	go func() { res <- FetchJSON(ctx, c, "http://svc.test/slow", nil) }()
	<-entered
	cancel()
	select {
	case err := <-res:
		if !errors.Is(err, context.Canceled) {
			t.Errorf("err = %v; want context.Canceled", err)
		}
	case <-time.After(3 * time.Second):
		t.Fatal("FetchJSON ignored cancellation; build the request with NewRequestWithContext")
	}
}

func TestFetchJSON_bad_url(t *testing.T) {
	c := NewTestClient(func(r *http.Request) (*http.Response, error) {
		t.Error("transport called for an invalid URL")
		return reply(200, "{}", nil), nil
	})
	if err := FetchJSON(context.Background(), c, "http://bad host/\x7f", nil); err == nil {
		t.Error("an invalid URL returned no error")
	}
}

func TestPostJSON(t *testing.T) {
	var method, ctype, body string
	c := NewTestClient(func(r *http.Request) (*http.Response, error) {
		method, ctype = r.Method, r.Header.Get("Content-Type")
		b, _ := io.ReadAll(r.Body)
		body = string(b)
		return reply(201, `{"n":42,"name":"made"}`, nil), nil
	})
	var out item
	if err := PostJSON(context.Background(), c, "http://svc.test/items", item{0, "made"}, &out); err != nil {
		t.Fatalf("PostJSON: %v", err)
	}
	if method != "POST" || ctype != "application/json" {
		t.Errorf("method=%q content-type=%q", method, ctype)
	}
	if body != `{"n":0,"name":"made"}` {
		t.Errorf("sent body %q", body)
	}
	if out != (item{42, "made"}) {
		t.Errorf("decoded %+v", out)
	}
}

func TestPostJSON_nil_out_and_errors(t *testing.T) {
	closed := false
	c := NewTestClient(func(r *http.Request) (*http.Response, error) { return reply(204, "", &closed), nil })
	if err := PostJSON(context.Background(), c, "http://svc.test/", map[string]int{"a": 1}, nil); err != nil {
		t.Errorf("nil out with empty body: %v", err)
	}
	if !closed {
		t.Error("body not closed")
	}
	c2 := NewTestClient(func(r *http.Request) (*http.Response, error) { return reply(422, "bad input", nil), nil })
	var he *HTTPError
	if err := PostJSON(context.Background(), c2, "http://svc.test/", 1, nil); !errors.As(err, &he) || he.Status != 422 || he.Body != "bad input" {
		t.Errorf("err = %v; want *HTTPError{422, bad input}", err)
	}
	if err := PostJSON(context.Background(), c2, "http://svc.test/", make(chan int), nil); err == nil {
		t.Error("a value that cannot be encoded returned no error")
	}
}
