package main

import (
	"fmt"
)

type Reloc struct {
	Site   int
	Symbol string
}

type Object struct {
	Code    []byte
	Exports map[string]int
	Relocs  []Reloc
}

func Link(objs []Object) ([]byte, error) { return nil, nil }

func main() {
	a := Object{Code: []byte{4, 0, 0, 0}, Exports: map[string]int{"main": 0}, Relocs: []Reloc{{Site: 1, Symbol: "f"}}}
	b := Object{Code: []byte{1, 2, 'h', 'i', 0}, Exports: map[string]int{"f": 0}}
	out, err := Link([]Object{a, b})
	fmt.Printf("% x (%v)\n", out, err)
}
