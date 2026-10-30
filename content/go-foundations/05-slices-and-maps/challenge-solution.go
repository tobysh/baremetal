package main

import (
	"fmt"
	"slices"
)

func GroupAnagrams(words []string) [][]string {
	groups := [][]string{}
	index := map[string]int{}
	for _, w := range words {
		letters := []rune(w)
		slices.Sort(letters)
		key := string(letters)
		i, ok := index[key]
		if !ok {
			i = len(groups)
			index[key] = i
			groups = append(groups, nil)
		}
		groups[i] = append(groups[i], w)
	}
	return groups
}

// Try it with Run.
func main() {
	fmt.Println(GroupAnagrams([]string{"listen", "silent", "enlist", "google", "gogole"}))
}
