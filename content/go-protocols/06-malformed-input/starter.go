package main

import (
	"bytes"
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

var ErrTruncated = errors.New("stream ended inside a frame")

type Scanner struct{}

func NewScanner() *Scanner { return &Scanner{} }

func (s *Scanner) Feed(chunk []byte) ([]Frame, []error) { return nil, nil }
func (s *Scanner) Close() ([]Frame, []error)            { return nil, nil }
func (s *Scanner) Buffered() int                        { return 0 }
func (s *Scanner) Dropped() int                         { return 0 }

var _ = bytes.IndexByte // you will want this

// Try it with Run: garbage and a damaged frame between two good ones.
func main() {
	a, _ := Encode(1, []byte("first"))
	b, _ := Encode(2, []byte("damaged"))
	b[6] ^= 0xFF
	c, _ := Encode(3, []byte("last"))
	stream := append(append(append(append([]byte("noise!"), a...), b...), []byte{1, 2, 3}...), c...)
	s := NewScanner()
	frames, errs := s.Feed(stream)
	more, e2 := s.Close()
	for _, f := range append(frames, more...) {
		fmt.Println("frame", f.Type, string(f.Payload))
	}
	for _, e := range append(errs, e2...) {
		fmt.Println("error:", e)
	}
	fmt.Println("dropped", s.Dropped())
}
