package main

import (
	"context"
	"fmt"
	"time"
)

func CallWithTimeout(d time.Duration, f func(ctx context.Context) (string, error)) (string, error) {
	return "", nil
}

func Remaining(ctx context.Context) (time.Duration, bool) {
	return 0, false
}

func Half(ctx context.Context) (context.Context, context.CancelFunc) {
	return ctx, func() {}
}

// Try it with Run.
func main() {
	s, err := CallWithTimeout(50*time.Millisecond, func(ctx context.Context) (string, error) {
		<-ctx.Done()
		return "", ctx.Err()
	})
	fmt.Printf("%q %v\n", s, err)
}
