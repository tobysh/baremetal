package main

import (
	"context"
	"errors"
	"fmt"
	"sync"
)

type Group struct {
	wg     sync.WaitGroup
	cancel context.CancelFunc
	once   sync.Once
	err    error
}

func WithContext(ctx context.Context) (*Group, context.Context) {
	ctx, cancel := context.WithCancel(ctx)
	return &Group{cancel: cancel}, ctx
}

func (g *Group) Go(f func() error) {
	g.wg.Add(1)
	go func() {
		defer g.wg.Done()
		if err := f(); err != nil {
			g.once.Do(func() {
				g.err = err
				if g.cancel != nil {
					g.cancel()
				}
			})
		}
	}()
}

func (g *Group) Wait() error {
	g.wg.Wait()
	if g.cancel != nil {
		g.cancel()
	}
	return g.err
}

func main() {
	g, _ := WithContext(context.Background())
	g.Go(func() error { return nil })
	g.Go(func() error { return errors.New("boom") })
	fmt.Println(g.Wait())
}
