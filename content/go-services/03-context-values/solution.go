package main

import (
	"context"
	"fmt"
)

type ctxKey int

const (
	requestIDKey ctxKey = iota
	userKey
)

func WithRequestID(ctx context.Context, id string) context.Context {
	return context.WithValue(ctx, requestIDKey, id)
}

func RequestID(ctx context.Context) (string, bool) {
	id, ok := ctx.Value(requestIDKey).(string)
	return id, ok
}

func WithUser(ctx context.Context, name string) context.Context {
	return context.WithValue(ctx, userKey, name)
}

func User(ctx context.Context) string {
	if u, ok := ctx.Value(userKey).(string); ok {
		return u
	}
	return "anonymous"
}

func Describe(ctx context.Context) string {
	id, ok := RequestID(ctx)
	if !ok {
		id = "-"
	}
	return fmt.Sprintf("req=%s user=%s", id, User(ctx))
}

func Detach(ctx context.Context) context.Context {
	return context.WithoutCancel(ctx)
}

func main() {
	ctx := WithUser(WithRequestID(context.Background(), "r-17"), "ada")
	fmt.Println(Describe(ctx))
}
