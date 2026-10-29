package main

import (
	"context"
	"fmt"
	"io"
	"net/http"
	"strings"
)

type RoundTripFunc func(*http.Request) (*http.Response, error)

func NewTestClient(fn RoundTripFunc) *http.Client {
	return &http.Client{}
}

type HTTPError struct {
	Status int
	Body   string
}

func (e *HTTPError) Error() string {
	return ""
}

func FetchJSON(ctx context.Context, c *http.Client, url string, out any) error {
	return nil
}

func PostJSON(ctx context.Context, c *http.Client, url string, in, out any) error {
	return nil
}

// Try it with Run.
func main() {
	c := NewTestClient(func(r *http.Request) (*http.Response, error) {
		return &http.Response{StatusCode: 200, Header: http.Header{}, Body: io.NopCloser(strings.NewReader(`{"n":1}`))}, nil
	})
	var v map[string]int
	fmt.Println(FetchJSON(context.Background(), c, "http://example.test/n", &v), v)
}
