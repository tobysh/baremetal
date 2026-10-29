package main

import (
	"context"
	"fmt"
	"time"
)

func SleepCtx(ctx context.Context, d time.Duration) error {
	return nil
}

func Ticks(ctx context.Context) <-chan int {
	return nil
}

func CallWithTimeout(d time.Duration, f func(ctx context.Context) (int, error)) (int, error) {
	return 0, nil
}

func main() {
	ctx, cancel := context.WithTimeout(context.Background(), 50*time.Millisecond)
	defer cancel()
	fmt.Println(SleepCtx(ctx, time.Second))
}
