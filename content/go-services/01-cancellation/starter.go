package main

import (
	"context"
	"fmt"
	"time"
)

func Counter(ctx context.Context) <-chan int {
	return nil
}

func Sleep(ctx context.Context, d time.Duration) error {
	return nil
}

func Drain(ctx context.Context, in <-chan int) []int {
	return nil
}

// Try it with Run.
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
