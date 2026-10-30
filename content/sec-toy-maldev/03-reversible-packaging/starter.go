package main

import (
	"fmt"
)

func Pack(plain []byte, key byte) ([]byte, error) { return nil, nil }
func Unpack(bundle []byte) ([]byte, error)        { return nil, nil }

func main() {
	b, err := Pack([]byte("inert payload"), 19)
	fmt.Printf("% x (%v)\n", b, err)
}
