package main

import (
	"encoding/binary"
	"fmt"
	"hash/crc32"
)

func Wrap(code []byte, entry uint16) ([]byte, error) {
	if len(code) == 0 || len(code) > 4096 || int(entry) >= len(code) {
		return nil, fmt.Errorf("invalid code range")
	}
	out := make([]byte, 12+len(code))
	copy(out, "BML1")
	binary.LittleEndian.PutUint16(out[4:6], entry)
	binary.LittleEndian.PutUint16(out[6:8], uint16(len(code)))
	binary.LittleEndian.PutUint32(out[8:12], crc32.ChecksumIEEE(code))
	copy(out[12:], code)
	return out, nil
}

func main() {
	b, err := Wrap([]byte{0}, 0)
	fmt.Printf("% x (%v)\n", b, err)
}
