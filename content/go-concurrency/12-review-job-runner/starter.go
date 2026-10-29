package main

import (
	"context"
	"fmt"
)

func RunJobs(ctx context.Context, limit int, jobs []func(ctx context.Context) (string, error)) ([]string, error) {
	return nil, nil
}

func main() {
	jobs := []func(ctx context.Context) (string, error){
		func(ctx context.Context) (string, error) { return "a", nil },
		func(ctx context.Context) (string, error) { return "b", nil },
	}
	fmt.Println(RunJobs(context.Background(), 2, jobs))
}
