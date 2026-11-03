package main

import "testing"

func TestReplacement(t *testing.T) {
	for old := 0; old < 8; old++ {
		for fresh := 0; fresh < 8; fresh++ {
			if old == fresh {
				continue
			}
			for _, value := range []int{-13, 0, 91} {
				ws := Replacement(old, fresh, value)
				if len(ws) != 4 {
					t.Fatal("writes")
				}
				kinds := []string{"alloc", "data", "inode", "free"}
				for i, w := range ws {
					b := fresh
					if i == 3 {
						b = old
					}
					v := 0
					if i == 1 {
						v = value
					}
					if w.Kind != kinds[i] || w.Block != b || w.Value != v {
						t.Fatal("write contract", i, w)
					}
				}
				for cut := 0; cut <= 4; cut++ {
					used := map[int]bool{old: true}
					data := map[int]int{old: 7}
					ptr := old
					for _, w := range ws[:cut] {
						switch w.Kind {
						case "alloc":
							used[w.Block] = true
						case "data":
							data[w.Block] = w.Value
						case "inode":
							ptr = w.Block
						case "free":
							delete(used, w.Block)
						}
					}
					if !used[ptr] {
						t.Fatal("dangling", cut)
					}
					v, ok := data[ptr]
					if !ok || (ptr == fresh && v != value) {
						t.Fatal("uninitialized", cut)
					}
				}
			}
		}
	}
}
