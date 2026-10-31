package main

import (
	"errors"
	"fmt"
)

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

func Encode(typ byte, payload []byte) ([]byte, error) {
	return nil, nil
}

func Decode(b []byte) (typ byte, payload []byte, n int, err error) {
	return 0, nil, 0, nil
}

// Try it with Run.
func main() {
	b, err := Encode(7, []byte("hi"))
	fmt.Printf("% x %v\n", b, err)
	typ, p, n, err := Decode(b[:5])
	fmt.Println(typ, p, n, err) // a prefix: ErrShort
	typ, p, n, err = Decode(b)
	fmt.Println(typ, string(p), n, err)
}
