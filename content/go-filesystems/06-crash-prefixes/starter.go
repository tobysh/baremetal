package main

import "fmt"

type State struct {
	Used []bool
	Ptr  int
}
type Write struct {
	Kind         string
	Block, Value int
}

func UnsafeCuts(initial State, writes []Write) []int { return nil }
func main() {
	fmt.Println(UnsafeCuts(State{Used: []bool{true, false}, Ptr: 0}, []Write{{"bit", 0, 0}, {"ptr", 0, 1}, {"bit", 1, 1}}))
}
