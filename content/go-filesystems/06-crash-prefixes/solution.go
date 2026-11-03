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

func UnsafeCuts(initial State, writes []Write) []int {
	s := State{Used: append([]bool(nil), initial.Used...), Ptr: initial.Ptr}
	out := []int{}
	for k := 0; k <= len(writes); k++ {
		if s.Ptr >= 0 && !s.Used[s.Ptr] {
			out = append(out, k)
		}
		if k < len(writes) {
			w := writes[k]
			if w.Kind == "ptr" {
				s.Ptr = w.Value
			} else {
				s.Used[w.Block] = w.Value == 1
			}
		}
	}
	return out
}
func main() {
	fmt.Println(UnsafeCuts(State{Used: []bool{true, false}, Ptr: 0}, []Write{{"bit", 0, 0}, {"ptr", 0, 1}, {"bit", 1, 1}}))
}
