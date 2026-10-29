package main

import (
	"context"
	"errors"
	"fmt"
	"time"
)

func Retry(ctx context.Context, attempts int, backoff, perAttempt time.Duration, f func(ctx context.Context) error) error {
	return nil
}

// Try it with Run.
func main() {
	n := 0
	err := Retry(context.Background(), 5, time.Millisecond, 0, func(ctx context.Context) error {
		n++
		if n < 3 {
			return errors.New("not yet")
		}
		return nil
	})
	fmt.Println(n, err)
}
