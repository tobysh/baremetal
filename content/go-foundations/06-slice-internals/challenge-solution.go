package main

import "fmt"

func Chunk(s []int, n int) [][]int {
	out := [][]int{}
	if n <= 0 {
		return out
	}
	for i := 0; i < len(s); i += n {
		end := i + n
		if end > len(s) {
			end = len(s)
		}
		out = append(out, s[i:end:end])
	}
	return out
}

// Try it with Run.
func main() {
	fmt.Println(Chunk([]int{1, 2, 3, 4, 5}, 2))
}
