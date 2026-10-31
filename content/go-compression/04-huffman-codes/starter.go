package main

import (
	"errors"
	"fmt"
)

// MaxCodeLen is the longest code this track supports (enough for any input under 2 GiB).
const MaxCodeLen = 48

var ErrBadLengths = errors.New("invalid code lengths")

type Code struct {
	Bits uint64 // the code, right-aligned: write the low Len bits, highest first
	Len  int    // 0 means the symbol is unused
}

// BuildLengths returns the Huffman code length of every symbol (0 when freq is 0).
// Tie-breaking: repeatedly merge the two lightest nodes, comparing nodes by
// (weight, smallest symbol they contain). A lone symbol gets length 1.
func BuildLengths(freq []int) []int { return nil }

// CanonicalCodes assigns canonical codes: sort used symbols by (length, symbol);
// codes count upwards, shifting left whenever the length grows. It returns
// ErrBadLengths if a length is negative or above MaxCodeLen, or if the lengths
// are oversubscribed (more codes than the bit space holds).
func CanonicalCodes(lengths []int) ([]Code, error) { return nil, nil }

func main() {
	freq := make([]int, 256)
	for _, c := range []byte("abracadabra") {
		freq[c]++
	}
	lengths := BuildLengths(freq)
	codes, _ := CanonicalCodes(lengths)
	for _, c := range []byte("abcdr") {
		fmt.Printf("%c: %0*b\n", c, codes[c].Len, codes[c].Bits)
	}
}
