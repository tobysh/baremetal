package main

import (
	"fmt"
)

func Wrap(code []byte, entry uint16) ([]byte, error) { return nil, nil }

func main() {
	b, err := Wrap([]byte{0}, 0)
	fmt.Printf("% x (%v)\n", b, err)
}
