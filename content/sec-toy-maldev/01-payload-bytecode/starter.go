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

func Assemble(program []Instruction) ([]byte, error) { return nil, nil }

func main() {
	b, err := Assemble([]Instruction{{Op: OpEmit, Text: "hi"}, {Op: OpWait, Ticks: 258}, {Op: OpHalt}})
	fmt.Printf("% x (%v)\n", b, err)
}
