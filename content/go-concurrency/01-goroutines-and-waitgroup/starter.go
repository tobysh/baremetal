package main

import "fmt"

// RunAll runs every task concurrently and returns when all are done.
func RunAll(tasks ...func()) {
}

// Squares returns []int{0, 1, 4, ...} of length n, one goroutine per element.
func Squares(n int) []int {
	return nil
}

func main() {
	fmt.Println(Squares(6))
}
