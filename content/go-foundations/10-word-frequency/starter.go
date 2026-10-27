package main

import "io"

type WordCount struct {
	Word  string
	Count int
}

// TopWords returns the n most frequent words in r.
func TopWords(r io.Reader, n int) []WordCount {
	return nil
}
