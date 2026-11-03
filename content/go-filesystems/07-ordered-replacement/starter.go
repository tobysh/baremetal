package main

import "fmt"

type Write struct {
	Kind         string
	Block, Value int
}

func Replacement(old, fresh, value int) []Write { return nil }
func main()                                     { fmt.Println(Replacement(3, 7, 42)) }
