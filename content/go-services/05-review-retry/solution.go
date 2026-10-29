package main

import (
	"context"
	"errors"
	"fmt"
	"time"
)

func Retry(ctx context.Context, attempts int, backoff, perAttempt time.Duration, f func(ctx context.Context) error) error {
	if attempts < 1 {
		attempts = 1
	}
	for i := 1; i <= attempts; i++ {
		if err := ctx.Err(); err != nil {
			return err
		}
		actx, cancel := ctx, func() {}
		if perAttempt > 0 {
			actx, cancel = context.WithTimeout(ctx, perAttempt)
		}
		err := f(actx)
		cancel()
		if err == nil {
			return nil
		}
		if i == attempts {
			return fmt.Errorf("after %d attempts: %w", i, err)
		}
		if backoff > 0 {
			t := time.NewTimer(backoff)
			select {
			case <-t.C:
			case <-ctx.Done():
				t.Stop()
				return ctx.Err()
			}
		}
	}
	return nil
}

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
