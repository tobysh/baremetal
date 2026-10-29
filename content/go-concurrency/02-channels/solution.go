package main

import "fmt"

func Gen(n int) <-chan int {
	ch := make(chan int)
	go func() {
		defer close(ch)
		for i := 1; i <= n; i++ {
			ch <- i
		}
	}()
	return ch
}

func Sum(ch <-chan int) int {
	total := 0
	for v := range ch {
		total += v
	}
	return total
}

func Preloaded(vals ...int) <-chan int {
	ch := make(chan int, len(vals))
	for _, v := range vals {
		ch <- v
	}
	close(ch)
	return ch
}

func Take(ch <-chan int, n int) []int {
	out := []int{}
	for len(out) < n {
		v, ok := <-ch
		if !ok {
			break
		}
		out = append(out, v)
	}
	return out
}

func main() {
	fmt.Println(Sum(Gen(10)))
}
