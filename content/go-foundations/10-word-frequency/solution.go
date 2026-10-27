package main

import (
	"bufio"
	"io"
	"sort"
	"strings"
	"unicode"
)

type WordCount struct {
	Word  string
	Count int
}

// TopWords returns the n most frequent words in r.
func TopWords(r io.Reader, n int) []WordCount {
	counts := make(map[string]int)
	sc := bufio.NewScanner(r)
	for sc.Scan() {
		words := strings.FieldsFunc(strings.ToLower(sc.Text()), func(r rune) bool {
			return !unicode.IsLetter(r) && r != '\''
		})
		for _, w := range words {
			counts[w]++
		}
	}

	items := make([]WordCount, 0, len(counts))
	for w, c := range counts {
		items = append(items, WordCount{Word: w, Count: c})
	}
	sort.Slice(items, func(i, j int) bool {
		if items[i].Count != items[j].Count {
			return items[i].Count > items[j].Count
		}
		return items[i].Word < items[j].Word
	})

	if n < 0 {
		n = 0
	}
	if n > len(items) {
		n = len(items)
	}
	return items[:n]
}
