package main

import (
	"fmt"
	"math/rand"
	"sort"
	"testing"
)

func TestPublic(t *testing.T) {
	b := []Edge{{"ann", "ops", "MemberOf"}, {"ops", "admins", "MemberOf"}}
	a := append(append([]Edge{}, b...), Edge{"tmp", "ops", "MemberOf"})
	names, en := Escalations(b, a, nil, "admins")
	if fmt.Sprint(names) != "[tmp]" || len(en) != 1 || en[0].Attributed {
		t.Fatal(names, en)
	}
}

func TestPlanted(t *testing.T) {
	for i := 0; i < 12; i++ {
		n := func(s string) string { return fmt.Sprint(s, i) }
		before := []Edge{
			{n("victim"), n("helpdesk"), "MemberOf"}, {n("helpdesk"), n("ops"), "MemberOf"}, {n("ops"), "admins", "MemberOf"},
			{n("dev1"), n("devs"), "MemberOf"}, {n("mgr"), n("dev1"), "Manages"}, {n("loop1"), n("loop2"), "MemberOf"}, {n("loop2"), n("loop1"), "MemberOf"},
		}
		after := append([]Edge{}, before...)
		add := func(e Edge) { after = append(after, e) }
		add(Edge{n("svc"), n("ops"), "MemberOf"})       // enabler, attributed twice
		add(Edge{n("svc"), n("ops"), "MemberOf"})       // duplicate
		add(Edge{n("newhire"), n("ops"), "MemberOf"})   // enabler by someone else, no audit
		add(Edge{n("chain1"), n("chain2"), "MemberOf"}) // two-hop
		add(Edge{n("chain2"), "admins", "HasRole"})
		add(Edge{n("svc2"), n("wiki"), "MemberOf"})  // no route
		add(Edge{n("victim"), "admins", "MemberOf"}) // redundant
		add(Edge{n("mgr"), n("ops"), "Manages"})     // ignored kind
		add(Edge{n("loop1"), n("devs"), "MemberOf"}) // no route
		add(Edge{n("loop2"), n("svc"), "MemberOf"})  // loop2 reaches admins via svc: loop1 too
		audit := []Audit{
			{100, n("victim"), "add", n("svc"), n("ops"), "MemberOf"},
			{50, n("other"), "add", n("svc"), n("ops"), "MemberOf"},
			{200, n("victim"), "add", n("chain1"), n("chain2"), "MemberOf"},
			{210, n("victim"), "add", n("chain2"), "admins", "HasRole"},
			{150, n("victim"), "remove", n("newhire"), n("ops"), "MemberOf"}, // remove is not an add
			{120, n("victim"), "add", n("loop2"), n("svc"), "MemberOf"},
		}
		rng := rand.New(rand.NewSource(int64(i)))
		rng.Shuffle(len(audit), func(a, b int) { audit[a], audit[b] = audit[b], audit[a] })
		rng.Shuffle(len(after), func(a, b int) { after[a], after[b] = after[b], after[a] })
		names, en := Escalations(before, after, audit, "admins")
		wantNames := []string{n("chain1"), n("chain2"), n("loop1"), n("loop2"), n("newhire"), n("svc")}
		sort.Strings(wantNames)
		wantEn := []Enabler{
			{Edge{n("chain1"), n("chain2"), "MemberOf"}, n("victim"), 200, true},
			{Edge{n("chain2"), "admins", "HasRole"}, n("victim"), 210, true},
			{Edge{n("loop2"), n("svc"), "MemberOf"}, n("victim"), 120, true},
			{Edge{n("newhire"), n("ops"), "MemberOf"}, "", 0, false},
			{Edge{n("svc"), n("ops"), "MemberOf"}, n("victim"), 100, true},
		}
		if fmt.Sprint(names) != fmt.Sprint(wantNames) {
			t.Fatalf("case %d names %v want %v", i, names, wantNames)
		}
		if fmt.Sprint(en) != fmt.Sprint(wantEn) {
			t.Fatalf("case %d enablers\n got %v\nwant %v", i, en, wantEn)
		}
	}
}

// Independent check on random graphs: Warshall closure.
func TestRandomAgainstClosure(t *testing.T) {
	rng := rand.New(rand.NewSource(99))
	kinds := []string{"MemberOf", "HasRole", "Manages"}
	for it := 0; it < 60; it++ {
		const N = 9
		node := func(i int) string {
			if i == 0 {
				return "T"
			}
			return fmt.Sprint("n", i)
		}
		gen := func() []Edge {
			var e []Edge
			for k := rng.Intn(14); k > 0; k-- {
				e = append(e, Edge{node(rng.Intn(N)), node(rng.Intn(N)), kinds[rng.Intn(3)]})
			}
			return e
		}
		b := gen()
		a := append(append([]Edge{}, b...), gen()...)
		clos := func(es []Edge) [N][N]bool {
			var m [N][N]bool
			idx := func(s string) int {
				if s == "T" {
					return 0
				}
				var x int
				fmt.Sscanf(s, "n%d", &x)
				return x
			}
			for _, e := range es {
				if e.Kind != "Manages" {
					m[idx(e.From)][idx(e.To)] = true
				}
			}
			for k := 0; k < N; k++ {
				for i := 0; i < N; i++ {
					for j := 0; j < N; j++ {
						if m[i][k] && m[k][j] {
							m[i][j] = true
						}
					}
				}
			}
			return m
		}
		cb, ca := clos(b), clos(a)
		var want []string
		for i := 1; i < N; i++ {
			if ca[i][0] && !cb[i][0] {
				want = append(want, node(i))
			}
		}
		got, _ := Escalations(b, a, nil, "T")
		if fmt.Sprint(got) != fmt.Sprint(want) && !(len(got) == 0 && len(want) == 0) {
			t.Fatalf("iter %d got %v want %v", it, got, want)
		}
	}
}
