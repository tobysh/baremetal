package main

import (
	"context"
	"fmt"
	"time"
)

func SleepCtx(ctx context.Context, d time.Duration) error {
	timer := time.NewTimer(d)
	defer timer.Stop()
	select {
	case <-timer.C:
		return nil
	case <-ctx.Done():
		return ctx.Err()
	}
}

func Ticks(ctx context.Context) <-chan int {
	out := make(chan int)
	go func() {
		defer close(out)
		for i := 0; ; i++ {
			select {
			case out <- i:
			case <-ctx.Done():
				return
			}
		}
	}()
	return out
}

func CallWithTimeout(d time.Duration, f func(ctx context.Context) (int, error)) (int, error) {
	ctx, cancel := context.WithTimeout(context.Background(), d)
	defer cancel()
	return f(ctx)
}

func main() {
	ctx, cancel := context.WithTimeout(context.Background(), 50*time.Millisecond)
	defer cancel()
	fmt.Println(SleepCtx(ctx, time.Second))
}
