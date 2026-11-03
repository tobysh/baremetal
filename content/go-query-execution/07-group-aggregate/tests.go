package main

import (
	"math/rand"
	"reflect"
	"testing"
)

func TestGroups(t *testing.T) {
	r := rand.New(rand.NewSource(78))
	for n := 0; n < 160; n++ {
		rows := make([]Row, n)
		for i := range rows {
			rows[i] = Row{Value{int64(r.Intn(7)), r.Intn(3) != 0}, Value{int64(r.Intn(61) - 30), r.Intn(3) != 0}}
		}
		before := append([]Row(nil), rows...)
		want := []Group{}
		for _, x := range rows {
			found := -1
			for i, g := range want {
				if g.Key.Valid == x.Key.Valid && (!g.Key.Valid || g.Key.N == x.Key.N) {
					found = i
					break
				}
			}
			if found < 0 {
				k := x.Key
				if !k.Valid {
					k.N = 0
				}
				want = append(want, Group{Key: k})
				found = len(want) - 1
			}
			want[found].Rows++
			if x.Value.Valid {
				want[found].Count++
				want[found].Sum.Valid = true
				want[found].Sum.N += x.Value.N
			}
		}
		got := Aggregate(rows)
		if len(got) != len(want) {
			t.Fatal("groups", n)
		}
		for i := range got {
			if got[i] != want[i] {
				t.Fatal("aggregate", n, i, got[i], want[i])
			}
		}
		if n > 0 && !reflect.DeepEqual(rows, before) {
			t.Fatal("input changed")
		}
	}
}
