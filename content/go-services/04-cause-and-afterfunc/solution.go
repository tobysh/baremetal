package main

import (
	"context"
	"errors"
	"fmt"
	"time"
)

func NewAbortable(parent context.Context) (context.Context, func(reason string)) {
	ctx, cancel := context.WithCancelCause(parent)
	return ctx, func(reason string) { cancel(errors.New(reason)) }
}

func Why(ctx context.Context) string {
	err := context.Cause(ctx)
	switch {
	case err == nil:
		return ""
	case errors.Is(err, context.Canceled):
		return "canceled"
	case errors.Is(err, context.DeadlineExceeded):
		return "timed out"
	default:
		return err.Error()
	}
}

func TimeoutWithCause(parent context.Context, d time.Duration, what string) (context.Context, context.CancelFunc) {
	return context.WithTimeoutCause(parent, d, fmt.Errorf("%s timed out", what))
}

func OnCancel(ctx context.Context, f func()) (stop func() bool) {
	return context.AfterFunc(ctx, f)
}

func main() {
	ctx, abort := NewAbortable(context.Background())
	abort("disk full")
	fmt.Println(Why(ctx))
}
