package main

import (
	"context"
	"fmt"
	"time"
)

func RateLimited(in <-chan int, every time.Duration) <-chan int {
	return nil
}

func Poll(ctx context.Context, every time.Duration, check func() bool) bool {
	return false
}

func main() {
	in := make(chan int, 3)
	in <- 1
	in <- 2
	in <- 3
	close(in)
	for v := range RateLimited(in, 100*time.Millisecond) {
		fmt.Println(v)
	}
}
