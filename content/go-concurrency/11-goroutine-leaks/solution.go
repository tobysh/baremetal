package main

import "fmt"

func First(fns ...func() int) int {
	if len(fns) == 0 {
		return 0
	}
	ch := make(chan int, len(fns))
	for _, f := range fns {
		go func() { ch <- f() }()
	}
	return <-ch
}

func Generator(done <-chan struct{}) <-chan int {
	out := make(chan int)
	go func() {
		defer close(out)
		for i := 0; ; i++ {
			select {
			case out <- i:
			case <-done:
				return
			}
		}
	}()
	return out
}

func main() {
	fmt.Println(First(func() int { return 1 }))
}
