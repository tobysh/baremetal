package main

import (
	"fmt"
	"sync"
)

func RunAll(tasks ...func()) {
	var wg sync.WaitGroup
	for _, t := range tasks {
		wg.Add(1)
		go func() {
			defer wg.Done()
			t()
		}()
	}
	wg.Wait()
}

func Squares(n int) []int {
	if n <= 0 {
		return []int{}
	}
	out := make([]int, n)
	var wg sync.WaitGroup
	for i := range out {
		wg.Add(1)
		go func() {
			defer wg.Done()
			out[i] = i * i
		}()
	}
	wg.Wait()
	return out
}

func main() {
	fmt.Println(Squares(6))
}
