package main

import (
	"fmt"
	"sync"
)

func Process(inputs []int, workers int, f func(int) int) []int {
	if workers < 1 {
		workers = 1
	}
	out := make([]int, len(inputs))
	jobs := make(chan int)
	var wg sync.WaitGroup
	for w := 0; w < workers; w++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			for i := range jobs {
				out[i] = f(inputs[i])
			}
		}()
	}
	for i := range inputs {
		jobs <- i
	}
	close(jobs)
	wg.Wait()
	return out
}

func Generate(nums ...int) <-chan int {
	out := make(chan int)
	go func() {
		defer close(out)
		for _, n := range nums {
			out <- n
		}
	}()
	return out
}

func Square(in <-chan int) <-chan int {
	out := make(chan int)
	go func() {
		defer close(out)
		for n := range in {
			out <- n * n
		}
	}()
	return out
}

func Collect(in <-chan int) []int {
	result := []int{}
	for v := range in {
		result = append(result, v)
	}
	return result
}

// Try it with Run.
func main() {
	fmt.Println(Collect(Square(Generate(1, 2, 3, 4))))
}
