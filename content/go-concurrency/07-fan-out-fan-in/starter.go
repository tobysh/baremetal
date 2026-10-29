package main

import "fmt"

func Merge(chans ...<-chan int) <-chan int {
	return nil
}

func FanOut(in <-chan int, workers int, f func(int) int) []<-chan int {
	return nil
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
