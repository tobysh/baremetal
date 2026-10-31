package main

import (
	"fmt"
	"math/rand"
	"testing"
)

func TestPublic(t *testing.T) {
	ev := []Event{{5, "svc", "login", "db0"}, {20, "svc", "login", "db1"}, {30, "key9", "read-secret", "vault"}}
	got := Scope("svc", 10, ev, map[string][]string{"db1": {"key9"}})
	want := []Item{{"svc", "credential", 10}, {"key9", "credential", 20}, {"db1", "host", 20}, {"vault", "credential", 30}}
	if fmt.Sprint(got) != fmt.Sprint(want) {
		t.Fatal(got)
	}
}

func TestSameSecond(t *testing.T) {
	// b reads c at t=50 listed BEFORE a reads b at t=50; all simultaneous.
	ev := []Event{{50, "b", "read-secret", "c"}, {50, "a", "read-secret", "b"}}
	got := Scope("a", 50, ev, nil)
	if len(got) != 3 {
		t.Fatal(got)
	}
}

// Planted chain: truth is the chain by construction; decoys cannot be reached.
func TestPlantedChains(t *testing.T) {
	for seed := int64(1); seed <= 20; seed++ {
		r := rand.New(rand.NewSource(seed))
		L := 2 + r.Intn(5)
		start := int64(1000 + r.Intn(500))
		var ev []Event
		stored := map[string][]string{}
		var want []Item
		cur := fmt.Sprint("cred0-", seed)
		want = append(want, Item{cur, "credential", start})
		tt := start
		for k := 1; k <= L; k++ {
			tt += int64(r.Intn(3)) * 10 // zero steps give same-second events
			next := fmt.Sprint("cred", k, "-", seed)
			if k%2 == 0 {
				h := fmt.Sprint("host", k, "-", seed)
				ev = append(ev, Event{tt, cur, "login", h})
				stored[h] = append(stored[h], next)
				want = append(want, Item{h, "host", tt})
			} else {
				ev = append(ev, Event{tt, cur, "read-secret", next})
			}
			want = append(want, Item{next, "credential", tt})
			// decoy: cur acted before compromise time on something else
			ev = append(ev, Event{start - 1 - int64(r.Intn(100)), cur, "login", fmt.Sprint("early", k, "-", seed)})
			cur = next
		}
		// decoys by never-compromised actors, and benign actions
		for k := 0; k < 6; k++ {
			ev = append(ev, Event{start + int64(r.Intn(200)), fmt.Sprint("svc", k, "-", seed), "login", fmt.Sprint("hostX", k, "-", seed)})
			ev = append(ev, Event{start + int64(r.Intn(200)), fmt.Sprint("svc", k, "-", seed), "read-secret", fmt.Sprint("vaultX", k, "-", seed)})
			ev = append(ev, Event{start + 5, cur, "logout", "ignored"})
		}
		stored[fmt.Sprint("hostX0-", seed)] = []string{"credY"}
		r.Shuffle(len(ev), func(i, j int) { ev[i], ev[j] = ev[j], ev[i] })
		got := Scope(fmt.Sprint("cred0-", seed), start, ev, stored)
		// order want canonically with the same rule the task states
		exp := canon(want)
		if fmt.Sprint(got) != fmt.Sprint(exp) {
			t.Fatalf("seed %d\n got %v\nwant %v", seed, got, exp)
		}
	}
}

func canon(items []Item) []Item {
	out := append([]Item{}, items...)
	for i := 1; i < len(out); i++ {
		for j := i; j > 0 && less(out[j], out[j-1]); j-- {
			out[j], out[j-1] = out[j-1], out[j]
		}
	}
	return out
}
func less(a, b Item) bool {
	if a.Since != b.Since {
		return a.Since < b.Since
	}
	if a.Kind != b.Kind {
		return a.Kind < b.Kind
	}
	return a.Name < b.Name
}
