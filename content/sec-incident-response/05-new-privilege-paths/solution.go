package main

import "sort"

type Edge struct{ From, To, Kind string }

type Audit struct {
	Time                          int64
	Actor, Action, From, To, Kind string
}

type Enabler struct {
	Edge       Edge
	Actor      string
	Time       int64
	Attributed bool
}

func counts(k string) bool { return k == "MemberOf" || k == "HasRole" }

func reach(edges []Edge, target string) map[string]bool {
	rev := map[string][]string{}
	for _, e := range edges {
		if counts(e.Kind) {
			rev[e.To] = append(rev[e.To], e.From)
		}
	}
	seen := map[string]bool{target: true}
	q := []string{target}
	for len(q) > 0 {
		n := q[0]
		q = q[1:]
		for _, f := range rev[n] {
			if !seen[f] {
				seen[f] = true
				q = append(q, f)
			}
		}
	}
	return seen // includes target
}

func Escalations(before, after []Edge, audit []Audit, target string) ([]string, []Enabler) {
	rb, ra := reach(before, target), reach(after, target)
	newly := map[string]bool{}
	names := []string{}
	for n := range ra {
		if n != target && !rb[n] {
			newly[n] = true
			names = append(names, n)
		}
	}
	sort.Strings(names)
	had := map[Edge]bool{}
	for _, e := range before {
		had[e] = true
	}
	done := map[Edge]bool{}
	en := []Enabler{}
	for _, e := range after {
		if !counts(e.Kind) || had[e] || done[e] || !newly[e.From] || !ra[e.To] {
			continue
		}
		done[e] = true
		x := Enabler{Edge: e}
		for _, a := range audit {
			if a.Action == "add" && a.From == e.From && a.To == e.To && a.Kind == e.Kind && (!x.Attributed || a.Time >= x.Time) {
				x.Actor, x.Time, x.Attributed = a.Actor, a.Time, true
			}
		}
		en = append(en, x)
	}
	sort.Slice(en, func(i, j int) bool {
		a, b := en[i].Edge, en[j].Edge
		if a.From != b.From {
			return a.From < b.From
		}
		if a.To != b.To {
			return a.To < b.To
		}
		return a.Kind < b.Kind
	})
	return names, en
}

func main() {}
