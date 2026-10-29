package main

import (
	"errors"
	"fmt"
)

var (
	ErrShort   = errors.New("frame is incomplete")
	ErrCorrupt = errors.New("frame checksum mismatch")
)

func Frame(payload []byte) []byte {
	return nil
}

func Unframe(data []byte) ([]byte, int, error) {
	return nil, 0, ErrShort
}

func ScanFrames(data []byte) ([][]byte, int, error) {
	return nil, 0, nil
}

// Try it with Run.
func main() {
	log := append(Frame([]byte("hello")), Frame([]byte("world"))...)
	log[len(log)-1] ^= 0x01 // flip one bit
	got, valid, err := ScanFrames(log)
	fmt.Printf("%q %d %v\n", got, valid, err)
}
