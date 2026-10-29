package main

import (
	"encoding/binary"
	"errors"
	"fmt"
	"hash/crc32"
)

var (
	ErrShort   = errors.New("frame is incomplete")
	ErrCorrupt = errors.New("frame checksum mismatch")
)

func Frame(payload []byte) []byte {
	f := make([]byte, 8+len(payload))
	binary.BigEndian.PutUint32(f[4:8], uint32(len(payload)))
	copy(f[8:], payload)
	binary.BigEndian.PutUint32(f[0:4], crc32.ChecksumIEEE(f[4:]))
	return f
}

func Unframe(data []byte) ([]byte, int, error) {
	if len(data) < 8 {
		return nil, 0, ErrShort
	}
	n := uint64(binary.BigEndian.Uint32(data[4:8]))
	if uint64(len(data)-8) < n {
		return nil, 0, ErrShort
	}
	total := 8 + int(n)
	if crc32.ChecksumIEEE(data[4:total]) != binary.BigEndian.Uint32(data[0:4]) {
		return nil, 0, ErrCorrupt
	}
	return data[8:total], total, nil
}

func ScanFrames(data []byte) ([][]byte, int, error) {
	var out [][]byte
	valid := 0
	for valid < len(data) {
		p, n, err := Unframe(data[valid:])
		if errors.Is(err, ErrShort) {
			break
		}
		if err != nil {
			return out, valid, fmt.Errorf("offset %d: %w", valid, err)
		}
		out = append(out, p)
		valid += n
	}
	return out, valid, nil
}

// Try it with Run.
func main() {
	log := append(Frame([]byte("hello")), Frame([]byte("world"))...)
	log[len(log)-1] ^= 0x01 // flip one bit
	got, valid, err := ScanFrames(log)
	fmt.Printf("%q %d %v\n", got, valid, err)
}
