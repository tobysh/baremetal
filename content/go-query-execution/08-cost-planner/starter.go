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

func Choose(s Stats) Plan { return Plan{} }
func main()               { fmt.Println(Choose(Stats{L: 1000, R: 2000, IndexRight: true, Matches: 1})) }
