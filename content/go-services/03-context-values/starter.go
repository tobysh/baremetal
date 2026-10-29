package main

import (
	"context"
	"fmt"
)

func WithRequestID(ctx context.Context, id string) context.Context {
	return ctx
}

func RequestID(ctx context.Context) (string, bool) {
	return "", false
}

func WithUser(ctx context.Context, name string) context.Context {
	return ctx
}

func User(ctx context.Context) string {
	return ""
}

func Describe(ctx context.Context) string {
	return ""
}

func Detach(ctx context.Context) context.Context {
	return ctx
}

// Try it with Run.
func main() {
	ctx := WithUser(WithRequestID(context.Background(), "r-17"), "ada")
	fmt.Println(Describe(ctx))
}
