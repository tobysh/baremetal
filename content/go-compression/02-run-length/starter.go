package main

import (
	"errors"
	"fmt"
)

var ErrTruncated = errors.New("input is truncated")

// RLEEncode: tokens are literal runs (control 0..127 = 1..128 raw bytes follow) and
// repeats (control 128..255 = 2..129 copies of the one byte that follows).
func RLEEncode(data []byte) []byte { return nil }

func RLEDecode(enc []byte) ([]byte, error) { return nil, ErrTruncated }

func main() {
	in := []byte("aaaaaabcdddddd")
	enc := RLEEncode(in)
	out, err := RLEDecode(enc)
	fmt.Printf("% x\n%q %v\n", enc, out, err)
}
