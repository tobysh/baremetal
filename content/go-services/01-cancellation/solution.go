package main

import (
	"context"
	"fmt"
	"time"
)

func Counter(ctx context.Context) <-chan int {
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

func Sleep(ctx context.Context, d time.Duration) error {
	if err := ctx.Err(); err != nil {
		return err
	}
	t := time.NewTimer(d)
	defer t.Stop()
	select {
	case <-t.C:
		return nil
	case <-ctx.Done():
		return ctx.Err()
	}
}

func Drain(ctx context.Context, in <-chan int) []int {
	result := []int{}
	for {
		select {
		case v, ok := <-in:
			if !ok {
				return result
			}
			result = append(result, v)
		case <-ctx.Done():
			return result
		}
	}
}

func main() {
	ctx, cancel := context.WithCancel(context.Background())
	for v := range Counter(ctx) {
		fmt.Println(v)
		if v == 2 {
			cancel()
		}
	}
	fmt.Println(Sleep(ctx, time.Second))
}
