package main

import (
	"errors"
	"fmt"
)

const MaxCodeLen = 48

var ErrBadLengths = errors.New("invalid code lengths")

type Code struct {
	Bits uint64
	Len  int
}

// BuildLengths: repeatedly merge the two lightest nodes, ordered by (weight, smallest symbol inside).
func BuildLengths(freq []int) []int {
	lengths := make([]int, len(freq))
	type node struct{ w, min, l, r int }
	var nodes []node
	var active []int
	for s, f := range freq {
		if f > 0 {
			nodes = append(nodes, node{f, s, -1, -1})
			active = append(active, len(nodes)-1)
		}
	}
	if len(active) == 0 {
		return lengths
	}
	if len(active) == 1 {
		lengths[nodes[active[0]].min] = 1
		return lengths
	}
	pick := func() int {
		best := 0
		for i := 1; i < len(active); i++ {
			x, y := nodes[active[i]], nodes[active[best]]
			if x.w < y.w || (x.w == y.w && x.min < y.min) {
				best = i
			}
		}
		n := active[best]
		active = append(active[:best], active[best+1:]...)
		return n
	}
	for len(active) > 1 {
		a, b := pick(), pick()
		m := nodes[a].min
		if nodes[b].min < m {
			m = nodes[b].min
		}
		nodes = append(nodes, node{nodes[a].w + nodes[b].w, m, a, b})
		active = append(active, len(nodes)-1)
	}
	var walk func(n, d int)
	walk = func(n, d int) {
		x := nodes[n]
		if x.l < 0 {
			lengths[x.min] = d
			return
		}
		walk(x.l, d+1)
		walk(x.r, d+1)
	}
	walk(active[0], 0)
	return lengths
}

func CanonicalCodes(lengths []int) ([]Code, error) {
	var count [MaxCodeLen + 1]int
	var kraft uint64
	for _, l := range lengths {
		if l < 0 || l > MaxCodeLen {
			return nil, ErrBadLengths
		}
		if l > 0 {
			count[l]++
			kraft += 1 << uint(MaxCodeLen-l)
			if kraft > 1<<MaxCodeLen {
				return nil, ErrBadLengths
			}
		}
	}
	var next [MaxCodeLen + 1]uint64
	code := uint64(0)
	for l := 1; l <= MaxCodeLen; l++ {
		code = (code + uint64(count[l-1])) << 1
		next[l] = code
	}
	codes := make([]Code, len(lengths))
	for s, l := range lengths {
		if l > 0 {
			codes[s] = Code{next[l], l}
			next[l]++
		}
	}
	return codes, nil
}

func main() {
	freq := make([]int, 256)
	for _, c := range []byte("abracadabra") {
		freq[c]++
	}
	lengths := BuildLengths(freq)
	codes, _ := CanonicalCodes(lengths)
	for _, c := range []byte("abcdr") {
		fmt.Printf("%c: %0*b\n", c, codes[c].Len, codes[c].Bits)
	}
}
