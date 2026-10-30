package main

import (
	"fmt"
)

type Instruction struct {
	Op    byte
	Text  string
	Ticks uint16
}

const (
	OpHalt byte = iota
	OpEmit
	OpRead
	OpWait
)

type Image struct {
	Instructions []Instruction
	Entry        int
}

func LoadCapsule(blob []byte) (Image, error) { return Image{}, nil }

func main() {
	fmt.Println(LoadCapsule([]byte{66, 77, 76, 49, 4, 0, 8, 0, 20, 28, 184, 70, 1, 2, 104, 105, 3, 2, 1, 0}))
}
