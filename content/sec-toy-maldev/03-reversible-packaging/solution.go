package main

import (
	"encoding/binary"
	"fmt"
	"hash/crc32"
)

func Pack(plain []byte, key byte) ([]byte, error) {
	if key == 0 || len(plain) == 0 || len(plain) > 8192 {
		return nil, fmt.Errorf("invalid package input")
	}
	out := make([]byte, 11+len(plain))
	copy(out, "BX01")
	binary.LittleEndian.PutUint16(out[4:6], uint16(len(plain)))
	out[6] = key
	binary.LittleEndian.PutUint32(out[7:11], crc32.ChecksumIEEE(plain))
	for i, b := range plain {
		out[11+i] = b ^ key
	}
	return out, nil
}
func Unpack(bundle []byte) ([]byte, error) {
	if len(bundle) < 11 || string(bundle[:4]) != "BX01" || bundle[6] == 0 {
		return nil, fmt.Errorf("invalid package header")
	}
	n := int(binary.LittleEndian.Uint16(bundle[4:6]))
	if n == 0 || n > 8192 || len(bundle) != 11+n {
		return nil, fmt.Errorf("invalid package length")
	}
	out := make([]byte, n)
	for i := range out {
		out[i] = bundle[11+i] ^ bundle[6]
	}
	if crc32.ChecksumIEEE(out) != binary.LittleEndian.Uint32(bundle[7:11]) {
		return nil, fmt.Errorf("package checksum mismatch")
	}
	return out, nil
}

func main() {
	b, err := Pack([]byte("inert payload"), 19)
	fmt.Printf("% x (%v)\n", b, err)
}
