package main

import (
	"fmt"
	"sort"
)

type Person struct {
	Name string
	Age  int
}

func SortPeople(ps []Person) {
	sort.Slice(ps, func(i, j int) bool {
		if ps[i].Age != ps[j].Age {
			return ps[i].Age < ps[j].Age
		}
		return ps[i].Name < ps[j].Name
	})
}

func TopWords(words []string, n int) []string {
	counts := map[string]int{}
	for _, w := range words {
		counts[w]++
	}
	distinct := make([]string, 0, len(counts))
	for w := range counts {
		distinct = append(distinct, w)
	}
	sort.Slice(distinct, func(i, j int) bool {
		a, b := distinct[i], distinct[j]
		if counts[a] != counts[b] {
			return counts[a] > counts[b]
		}
		return a < b
	})
	if n < 0 {
		n = 0
	}
	if n > len(distinct) {
		n = len(distinct)
	}
	return distinct[:n]
}

func Counter() func() int {
	n := 0
	return func() int {
		n++
		return n
	}
}

func Memoize(f func(int) int) func(int) int {
	cache := map[int]int{}
	return func(x int) int {
		if v, ok := cache[x]; ok {
			return v
		}
		v := f(x)
		cache[x] = v
		return v
	}
}

// Try it with Run.
func main() {
	next := Counter()
	fmt.Println(next(), next(), next())
	fmt.Println(TopWords([]string{"b", "a", "b", "c", "a", "b"}, 2))
}
