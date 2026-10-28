package main

import "fmt"

func Process(inputs []int, workers int, f func(int) int) []int {
	return nil
}

func Generate(nums ...int) <-chan int {
	return nil
}

func Square(in <-chan int) <-chan int {
	return nil
}

func Collect(in <-chan int) []int {
	return nil
}

// Try it with Run.
func main() {
	fmt.Println(Collect(Square(Generate(1, 2, 3, 4))))
}
