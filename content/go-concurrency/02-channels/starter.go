package main

import "fmt"

func Gen(n int) <-chan int {
	return nil
}

func Sum(ch <-chan int) int {
	return 0
}

func Preloaded(vals ...int) <-chan int {
	return nil
}

func Take(ch <-chan int, n int) []int {
	return nil
}

func main() {
	fmt.Println(Sum(Gen(10)))
}
