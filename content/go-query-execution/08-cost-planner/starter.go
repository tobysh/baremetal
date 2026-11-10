package main

import "fmt"

// ---- Provided: the planner's input and output ------------------------------

// Stats are the (given, not measured) estimates for one join query.
//   L, R        row counts of the left and right inputs
//   Matches     expected matching right rows per left row (index join)
//   Selected    left rows an index range scan would return
//   Sorted*     the input already arrives sorted on the join key
//   Index*      an index on the join key / filter column exists
type Stats struct {
	L, R, Matches, Selected                        int64
	SortedLeft, SortedRight, IndexLeft, IndexRight bool
}

// Plan is the decision: Join is "nested", "hash", "merge" or "index";
// Access is "scan" or "index" for the left table; Cost is the join's cost.
type Plan struct {
	Join, Access string
	Cost         int64
}

// ---- Your work -----------------------------------------------------------

// Choose returns the cheapest join (ties: nested, hash, merge, index) and the
// access path for the left table. The formulas are in task.md.
//
// Plan: start with nested as best, then try each other candidate in tie order
// and take it only when strictly cheaper. Keep every value int64.
func Choose(s Stats) Plan { return Plan{} }

// main is not part of the exercise. Expected once implemented:
// {index scan 3000}
func main() { fmt.Println(Choose(Stats{L: 1000, R: 2000, IndexRight: true, Matches: 1})) }
