package main

import (
	"context"
	"fmt"
	"time"
)

type result struct {
	s   string
	err error
}

func CallWithTimeout(d time.Duration, f func(ctx context.Context) (string, error)) (string, error) {
	ctx, cancel := context.WithTimeout(context.Background(), d)
	defer cancel()
	ch := make(chan result, 1)
	go func() {
		s, err := f(ctx)
		ch <- result{s, err}
	}()
	select {
	case r := <-ch:
		return r.s, r.err
	case <-ctx.Done():
		return "", ctx.Err()
	}
}

func Remaining(ctx context.Context) (time.Duration, bool) {
	dl, ok := ctx.Deadline()
	if !ok {
		return 0, false
	}
	left := time.Until(dl)
	if left < 0 {
		left = 0
	}
	return left, true
}

func Half(ctx context.Context) (context.Context, context.CancelFunc) {
	left, ok := Remaining(ctx)
	if !ok {
		return context.WithCancel(ctx)
	}
	return context.WithTimeout(ctx, left/2)
}

func main() {
	s, err := CallWithTimeout(50*time.Millisecond, func(ctx context.Context) (string, error) {
		<-ctx.Done()
		return "", ctx.Err()
	})
	fmt.Printf("%q %v\n", s, err)
}
