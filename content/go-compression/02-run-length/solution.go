package main

import (
	"errors"
	"fmt"
)

var ErrTruncated = errors.New("input is truncated")

// RLEEncode: tokens are literal runs (control 0..127 = 1..128 raw bytes follow) and
// repeats (control 128..255 = 2..129 copies of the one byte that follows).
// A run of 3 or more equal bytes becomes a repeat (at most 129 per token); everything
// else is collected into literals (at most 128 per token).
func RLEEncode(data []byte) []byte {
	var out, lit []byte
	flush := func() {
		if len(lit) > 0 {
			out = append(out, byte(len(lit)-1))
			out = append(out, lit...)
			lit = lit[:0]
		}
	}
	for i := 0; i < len(data); {
		r := 1
		for i+r < len(data) && data[i+r] == data[i] && r < 129 {
			r++
		}
		if r >= 3 {
			flush()
			out = append(out, byte(128+r-2), data[i])
			i += r
			continue
		}
		lit = append(lit, data[i])
		i++
		if len(lit) == 128 {
			flush()
		}
	}
	flush()
	return out
}

func RLEDecode(enc []byte) ([]byte, error) {
	var out []byte
	for i := 0; i < len(enc); {
		c := int(enc[i])
		i++
		if c < 128 {
			n := c + 1
			if i+n > len(enc) {
				return nil, ErrTruncated
			}
			out = append(out, enc[i:i+n]...)
			i += n
		} else {
			if i >= len(enc) {
				return nil, ErrTruncated
			}
			for k := 0; k < c-128+2; k++ {
				out = append(out, enc[i])
			}
			i++
		}
	}
	return out, nil
}

func main() {
	in := []byte("aaaaaabcdddddd")
	enc := RLEEncode(in)
	out, err := RLEDecode(enc)
	fmt.Printf("% x\n%q %v\n", enc, out, err)
}
