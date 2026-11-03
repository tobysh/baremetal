package main

import "fmt"
import "sort"

type Image struct {
	NBlocks, DataStart int
	Used               []bool
	Inodes             [][]int
}
type Report struct {
	BadGeometry                          bool
	Orphan, Dangling, Duplicate, Invalid []int
}

func Check(im Image) Report { return Report{} }
func main() {
	fmt.Printf("%+v\n", Check(Image{NBlocks: 5, DataStart: 2, Used: []bool{true, true, true, false, true}, Inodes: [][]int{{2, 3}, {2}}}))
}

// Supplied deterministic report ordering.
func sortBlocks(blocks []int) { sort.Ints(blocks) }
