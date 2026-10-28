package main

import (
	"cmp"
	"fmt"
)

func Map[T, U any](xs []T, f func(T) U) []U {
	return nil
}

func Filter[T any](xs []T, keep func(T) bool) []T {
	return nil
}

func Reduce[T, A any](xs []T, init A, f func(A, T) A) A {
	var zero A
	return zero
}

func Max[T cmp.Ordered](xs []T) (T, bool) {
	var zero T
	return zero, false
}

type Stack[T any] struct {
}

func (s *Stack[T]) Push(x T) {
}

func (s *Stack[T]) Pop() (T, bool) {
	var zero T
	return zero, false
}

func (s *Stack[T]) Len() int {
	return 0
}

// Try it with Run.
func main() {
	fmt.Println(Map([]int{1, 2, 3}, func(n int) int { return n * n }))
}
