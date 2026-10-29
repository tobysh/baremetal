package main

import (
	"context"
	"fmt"
	"time"
)

func NewAbortable(parent context.Context) (context.Context, func(reason string)) {
	return parent, func(string) {}
}

func Why(ctx context.Context) string {
	return ""
}

func TimeoutWithCause(parent context.Context, d time.Duration, what string) (context.Context, context.CancelFunc) {
	return context.WithCancel(parent)
}

func OnCancel(ctx context.Context, f func()) (stop func() bool) {
	return func() bool { return false }
}

// Try it with Run.
func main() {
	ctx, abort := NewAbortable(context.Background())
	abort("disk full")
	fmt.Println(Why(ctx))
}
