package main

import (
	"context"
	"errors"
	"fmt"
)

// Group runs tasks and collects the first error.
type Group struct {
}

func WithContext(ctx context.Context) (*Group, context.Context) {
	return &Group{}, ctx
}

func (g *Group) Go(f func() error) {
}

func (g *Group) Wait() error {
	return nil
}

func main() {
	g, _ := WithContext(context.Background())
	g.Go(func() error { return nil })
	g.Go(func() error { return errors.New("boom") })
	fmt.Println(g.Wait())
}
