package main

import (
	"errors"
	"fmt"
)

var ErrTruncated = errors.New("input is truncated")

var ErrCorrupt = errors.New("corrupt data")

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

var ErrTooLarge = errors.New("output exceeds the limit")

type Limits struct {
	MaxOutput  int // bytes
	MaxRatio   int // output may not exceed MaxRatio * input bytes consumed (0 = no ratio check)
	RatioFloor int // the ratio is only enforced once the output is larger than this
}

// RLEDecodeLimited is RLEDecode that refuses to grow past the limits, and
// refuses BEFORE it allocates or copies the oversized token.
func RLEDecodeLimited(enc []byte, lim Limits) ([]byte, error) { return nil, ErrTruncated }

// Pack: mode byte 0 + raw data, or mode byte 1 + RLEEncode(data) if that is strictly shorter than data.
func Pack(data []byte) []byte { return nil }

// Unpack reverses Pack under the limits.
func Unpack(enc []byte, lim Limits) ([]byte, error) { return nil, ErrTruncated }

func main() {
	zeros := make([]byte, 10000)
	p := Pack(zeros)
	fmt.Println("10000 zeros pack to", len(p), "bytes")
	_, err := Unpack(p, Limits{MaxOutput: 1000})
	fmt.Println("with MaxOutput 1000:", err)
	out, err := Unpack(p, Limits{MaxOutput: 10000})
	fmt.Println("with MaxOutput 10000:", len(out), err)
}
