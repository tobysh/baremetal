package main

import "fmt"

type Stats struct {
	L, R, Matches, Selected                        int64
	SortedLeft, SortedRight, IndexLeft, IndexRight bool
}
type Plan struct {
	Join, Access string
	Cost         int64
}

func Choose(s Stats) Plan {
	p := Plan{Join: "nested", Access: "scan", Cost: s.L * s.R}
	cost := 3 * (s.L + s.R)
	if cost < p.Cost {
		p.Join = "hash"
		p.Cost = cost
	}
	cost = s.L + s.R
	if !s.SortedLeft {
		cost += 4 * s.L
	}
	if !s.SortedRight {
		cost += 4 * s.R
	}
	if cost < p.Cost {
		p.Join = "merge"
		p.Cost = cost
	}
	if s.IndexRight {
		cost = s.L * (2 + s.Matches)
		if cost < p.Cost {
			p.Join = "index"
			p.Cost = cost
		}
	}
	if s.IndexLeft && 2+s.Selected < s.L {
		p.Access = "index"
	}
	return p
}
func main() { fmt.Println(Choose(Stats{L: 1000, R: 2000, IndexRight: true, Matches: 1})) }
