package main

import (
	"fmt"
	"sync"
)

func Merge(chans ...<-chan int) <-chan int {
	out := make(chan int)
	var wg sync.WaitGroup
	wg.Add(len(chans))
	for _, c := range chans {
		go func() {
			defer wg.Done()
			for v := range c {
				out <- v
			}
		}()
	}
	go func() {
		wg.Wait()
		close(out)
	}()
	return out
}

func FanOut(in <-chan int, workers int, f func(int) int) []<-chan int {
	if workers < 1 {
		workers = 1
	}
	outs := make([]<-chan int, workers)
	for i := 0; i < workers; i++ {
		o := make(chan int)
		outs[i] = o
		go func() {
			defer close(o)
			for v := range in {
				o <- f(v)
			}
		}()
	}
	return outs
}

func gen(nums ...int) <-chan int {
	out := make(chan int)
	go func() {
		defer close(out)
		for _, n := range nums {
			out <- n
		}
	}()
	return out
}

func main() {
	total := 0
	for v := range Merge(FanOut(gen(1, 2, 3, 4, 5), 3, func(n int) int { return n * n })...) {
		total += v
	}
	fmt.Println(total)
}
