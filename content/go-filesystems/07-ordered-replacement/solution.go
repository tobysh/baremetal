package main

import "fmt"

type Write struct {
	Kind         string
	Block, Value int
}

func Replacement(old, fresh, value int) []Write {
	return []Write{{"alloc", fresh, 0}, {"data", fresh, value}, {"inode", fresh, 0}, {"free", old, 0}}
}
func main() { fmt.Println(Replacement(3, 7, 42)) }
