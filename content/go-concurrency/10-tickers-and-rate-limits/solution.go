package main

import (
	"context"
	"fmt"
	"time"
)

func RateLimited(in <-chan int, every time.Duration) <-chan int {
	out := make(chan int)
	go func() {
		defer close(out)
		t := time.NewTicker(every)
		defer t.Stop()
		for v := range in {
			<-t.C
			out <- v
		}
	}()
	return out
}

func Poll(ctx context.Context, every time.Duration, check func() bool) bool {
	t := time.NewTicker(every)
	defer t.Stop()
	for {
		select {
		case <-t.C:
			if check() {
				return true
			}
		case <-ctx.Done():
			return false
		}
	}
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
