package main

import "fmt"

type Person struct {
	Name string
	Age  int
}

func SortPeople(ps []Person) {
}

func TopWords(words []string, n int) []string {
	return nil
}

func Counter() func() int {
	return func() int { return 0 }
}

func Memoize(f func(int) int) func(int) int {
	return f
}

// Try it with Run.
func main() {
	next := Counter()
	fmt.Println(next(), next(), next())
	fmt.Println(TopWords([]string{"b", "a", "b", "c", "a", "b"}, 2))
}
