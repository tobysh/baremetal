package main

import (
	"errors"
	"fmt"
	"hash/crc32"
)

// ---- given: the codec from lesson 1 ----

const (
	Magic      = 0xBE
	HeaderLen  = 4
	TrailerLen = 4
	MaxPayload = 1024
)

var (
	ErrShort    = errors.New("frame incomplete: need more bytes")
	ErrMagic    = errors.New("bad magic byte")
	ErrTooLarge = errors.New("payload too large")
	ErrChecksum = errors.New("checksum mismatch")
)

type Frame struct {
	Type    byte
	Payload []byte
}

// Encode builds [Magic][type][len hi][len lo][payload][crc32 BE of everything before it].
func Encode(typ byte, payload []byte) ([]byte, error) {
	if len(payload) > MaxPayload {
		return nil, ErrTooLarge
	}
	b := make([]byte, 0, HeaderLen+len(payload)+TrailerLen)
	b = append(b, Magic, typ, byte(len(payload)>>8), byte(len(payload)))
	b = append(b, payload...)
	s := crc32.ChecksumIEEE(b)
	return append(b, byte(s>>24), byte(s>>16), byte(s>>8), byte(s)), nil
}

// Decode reads one frame from the start of b. It returns ErrShort while more
// bytes are needed, and never reads past what the header promises.
func Decode(b []byte) (typ byte, payload []byte, n int, err error) {
	if len(b) == 0 {
		return 0, nil, 0, ErrShort
	}
	if b[0] != Magic {
		return 0, nil, 0, ErrMagic
	}
	if len(b) < HeaderLen {
		return 0, nil, 0, ErrShort
	}
	l := int(b[2])<<8 | int(b[3])
	if l > MaxPayload {
		return 0, nil, 0, ErrTooLarge
	}
	total := HeaderLen + l + TrailerLen
	if len(b) < total {
		return 0, nil, 0, ErrShort
	}
	want := uint32(b[total-4])<<24 | uint32(b[total-3])<<16 | uint32(b[total-2])<<8 | uint32(b[total-1])
	if crc32.ChecksumIEEE(b[:total-4]) != want {
		return 0, nil, 0, ErrChecksum
	}
	p := make([]byte, l)
	copy(p, b[HeaderLen:HeaderLen+l])
	return b[1], p, total, nil
}

// ---- your work ----

type Parser struct {
	buf []byte
	err error
}

func NewParser() *Parser { return &Parser{} }

func (p *Parser) Feed(chunk []byte) ([]Frame, error) {
	if p.err != nil {
		return nil, p.err
	}
	p.buf = append(p.buf, chunk...)
	var out []Frame
	for {
		typ, payload, n, err := Decode(p.buf)
		if err == ErrShort {
			break
		}
		if err != nil {
			p.err, p.buf = err, nil
			return out, err
		}
		out = append(out, Frame{typ, payload})
		p.buf = p.buf[n:]
	}
	return out, nil
}

func (p *Parser) Buffered() int { return len(p.buf) }

// Try it with Run: the same bytes, two different chunkings.
func main() {
	a, _ := Encode(1, []byte("alpha"))
	b, _ := Encode(2, []byte("beta"))
	stream := append(a, b...)
	for _, cut := range []int{len(stream), 3, 9} {
		p := NewParser()
		var got []string
		for i := 0; i < len(stream); i += cut {
			end := min(i+cut, len(stream))
			frames, err := p.Feed(stream[i:end])
			for _, f := range frames {
				got = append(got, fmt.Sprintf("%d:%s", f.Type, f.Payload))
			}
			if err != nil {
				fmt.Println("error:", err)
			}
		}
		fmt.Println("chunks of", cut, "->", got)
	}
}
