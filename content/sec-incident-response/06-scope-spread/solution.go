package main

import "sort"

type Event struct {
	Time                  int64
	Actor, Action, Object string
}

type Item struct {
	Name, Kind string
	Since      int64
}

func Scope(startCred string, startTime int64, events []Event, stored map[string][]string) []Item {
	cred := map[string]int64{startCred: startTime}
	host := map[string]int64{}
	lower := func(m map[string]int64, k string, t int64) bool {
		if old, ok := m[k]; ok && old <= t {
			return false
		}
		m[k] = t
		return true
	}
	for changed := true; changed; {
		changed = false
		for _, e := range events {
			since, ok := cred[e.Actor]
			if !ok || since > e.Time {
				continue
			}
			switch e.Action {
			case "login":
				if lower(host, e.Object, e.Time) {
					changed = true
				}
				for _, c := range stored[e.Object] {
					if lower(cred, c, e.Time) {
						changed = true
					}
				}
			case "read-secret":
				if lower(cred, e.Object, e.Time) {
					changed = true
				}
			}
		}
	}
	out := []Item{}
	for n, t := range cred {
		out = append(out, Item{n, "credential", t})
	}
	for n, t := range host {
		out = append(out, Item{n, "host", t})
	}
	sort.Slice(out, func(i, j int) bool {
		a, b := out[i], out[j]
		if a.Since != b.Since {
			return a.Since < b.Since
		}
		if a.Kind != b.Kind {
			return a.Kind < b.Kind
		}
		return a.Name < b.Name
	})
	return out
}

func main() {}
