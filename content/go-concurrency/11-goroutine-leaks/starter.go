package main

import "fmt"

func First(fns ...func() int) int {
	return 0
}

func Generator(done <-chan struct{}) <-chan int {
	return nil
}

func main() {
	fmt.Println(First(func() int { return 1 }))
}
