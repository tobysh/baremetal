package main

import "fmt"

// ---- Provided: the data model --------------------------------------------

// Value is a nullable int64. Valid == false means SQL NULL.
type Value struct {
	N     int64
	Valid bool
}

// Row is one input row: the GROUP BY key and the value being aggregated.
type Row struct{ Key, Value Value }

// Group is one output row of
//   SELECT Key, COUNT(*), COUNT(Value), SUM(Value) ... GROUP BY Key
// Rows is COUNT(*), Count is COUNT(Value), Sum is SUM(Value) (NULL when no
// value in the group was valid).
type Group struct {
	Key         Value
	Rows, Count int
	Sum         Value
}

// ---- Your work -----------------------------------------------------------

// Aggregate returns one Group per distinct key, in order of first appearance.
// All NULL keys form one group. Do not modify rows.
//
// Plan: keep a []Group for the order and a map from key to position in it.
// Remember that two NULL keys can differ in N, so normalise them first.
func Aggregate(rows []Row) []Group { return nil }

// main is not part of the exercise; it shows one group of two rows with a
// single non-NULL value. Expected once implemented: [{{1 true} 2 1 {7 true}}]
func main() {
	fmt.Println(Aggregate([]Row{{Key: Value{N: 1, Valid: true}, Value: Value{N: 7, Valid: true}}, {Key: Value{N: 1, Valid: true}}}))
}
