package main

import "fmt"

type Value struct {
	N     int64
	Valid bool
}
type Row struct{ Key, Value Value }
type Group struct {
	Key         Value
	Rows, Count int
	Sum         Value
}

func Aggregate(rows []Row) []Group { return nil }
func main() {
	fmt.Println(Aggregate([]Row{{Key: Value{N: 1, Valid: true}, Value: Value{N: 7, Valid: true}}, {Key: Value{N: 1, Valid: true}}}))
}
