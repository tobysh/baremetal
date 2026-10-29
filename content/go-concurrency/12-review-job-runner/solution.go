package main

import (
	"context"
	"fmt"
	"sync"
)

func RunJobs(parent context.Context, limit int, jobs []func(ctx context.Context) (string, error)) ([]string, error) {
	if limit < 1 {
		limit = 1
	}
	results := make([]string, len(jobs))
	ctx, cancel := context.WithCancel(parent)
	defer cancel()

	var (
		mu       sync.Mutex
		firstErr error
		skipped  bool
		wg       sync.WaitGroup
	)
	sem := make(chan struct{}, limit)

loop:
	for i, job := range jobs {
		select {
		case sem <- struct{}{}:
		case <-ctx.Done():
			skipped = true
			break loop
		}
		if ctx.Err() != nil {
			<-sem
			skipped = true
			break
		}
		wg.Add(1)
		go func() {
			defer wg.Done()
			defer func() { <-sem }()
			s, err := job(ctx)
			if err != nil {
				mu.Lock()
				if firstErr == nil {
					firstErr = err
					cancel()
				}
				mu.Unlock()
				return
			}
			results[i] = s
		}()
	}
	wg.Wait()

	mu.Lock()
	defer mu.Unlock()
	if firstErr != nil {
		return results, firstErr
	}
	if skipped {
		return results, parent.Err()
	}
	return results, nil
}

func main() {
	jobs := []func(ctx context.Context) (string, error){
		func(ctx context.Context) (string, error) { return "a", nil },
		func(ctx context.Context) (string, error) { return "b", nil },
	}
	fmt.Println(RunJobs(context.Background(), 2, jobs))
}
