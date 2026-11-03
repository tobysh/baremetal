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

func Check(im Image) Report {
	r := Report{}
	if im.NBlocks < 0 || im.DataStart < 0 || im.DataStart > im.NBlocks || len(im.Used) != im.NBlocks {
		r.BadGeometry = true
		return r
	}
	counts := make([]int, im.NBlocks)
	for _, inode := range im.Inodes {
		for _, b := range inode {
			if b < im.DataStart || b >= im.NBlocks {
				r.Invalid = append(r.Invalid, b)
			} else {
				counts[b]++
			}
		}
	}
	sort.Ints(r.Invalid)
	for b := im.DataStart; b < im.NBlocks; b++ {
		if counts[b] == 0 && im.Used[b] {
			r.Orphan = append(r.Orphan, b)
		}
		if counts[b] > 0 && !im.Used[b] {
			r.Dangling = append(r.Dangling, b)
		}
		if counts[b] > 1 {
			r.Duplicate = append(r.Duplicate, b)
		}
	}
	return r
}
func main() {
	fmt.Printf("%+v\n", Check(Image{NBlocks: 5, DataStart: 2, Used: []bool{true, true, true, false, true}, Inodes: [][]int{{2, 3}, {2}}}))
}

// Supplied deterministic report ordering.
func sortBlocks(blocks []int) { sort.Ints(blocks) }
