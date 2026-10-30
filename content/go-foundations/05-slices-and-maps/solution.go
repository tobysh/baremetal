package main

import "strings"

// WordCount counts words case-insensitively.
func WordCount(text string) map[string]int {
	counts := make(map[string]int)
	for _, w := range strings.Fields(strings.ToLower(text)) {
		counts[w]++
	}
	return counts
}

// Dedup removes duplicates, keeping first occurrences in order.
func Dedup(nums []int) []int {
	seen := make(map[int]bool)
	result := []int{}
	for _, n := range nums {
		if !seen[n] {
			seen[n] = true
			result = append(result, n)
		}
	}
	return result
}
