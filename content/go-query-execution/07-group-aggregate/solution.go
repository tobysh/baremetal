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

func Aggregate(rows []Row) []Group {
	out := []Group{}
	index := map[Value]int{}
	for _, r := range rows {
		k := r.Key
		if !k.Valid {
			k.N = 0
		}
		i, ok := index[k]
		if !ok {
			i = len(out)
			index[k] = i
			out = append(out, Group{Key: k})
		}
		g := &out[i]
		g.Rows++
		if r.Value.Valid {
			g.Count++
			g.Sum.Valid = true
			g.Sum.N += r.Value.N
		}
	}
	return out
}
func main() {
	fmt.Println(Aggregate([]Row{{Key: Value{N: 1, Valid: true}, Value: Value{N: 7, Valid: true}}, {Key: Value{N: 1, Valid: true}}}))
}
