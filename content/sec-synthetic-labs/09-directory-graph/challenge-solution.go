package main

import (
	"encoding/json"
	"fmt"
	"sort"
)

type Node struct{ ID, Type, Label string }
type Edge struct{ From, To, Kind string }
type Directory struct {
	Nodes []Node
	Edges []Edge
}

func AdminPath(raw, start, goal string) ([]string, error) {
	var d Directory
	if err := json.Unmarshal([]byte(raw), &d); err != nil {
		return nil, err
	}
	nodes := map[string]bool{}
	for _, n := range d.Nodes {
		if n.ID == "" || nodes[n.ID] {
			return nil, fmt.Errorf("invalid node ID")
		}
		nodes[n.ID] = true
	}
	if !nodes[start] || !nodes[goal] {
		return nil, fmt.Errorf("missing endpoint")
	}
	adj := map[string][]string{}
	for _, e := range d.Edges {
		if !nodes[e.From] || !nodes[e.To] {
			return nil, fmt.Errorf("unknown edge endpoint")
		}
		switch e.Kind {
		case "MemberOf", "GenericAll", "AdminTo", "HasSession":
			adj[e.From] = append(adj[e.From], e.To)
		}
	}
	for n := range adj {
		sort.Strings(adj[n])
	}
	queue := []string{start}
	seen := map[string]bool{start: true}
	parent := map[string]string{}
	for len(queue) > 0 {
		n := queue[0]
		queue = queue[1:]
		if n == goal {
			out := []string{}
			for cur := goal; ; cur = parent[cur] {
				out = append(out, cur)
				if cur == start {
					break
				}
			}
			for i, j := 0, len(out)-1; i < j; i, j = i+1, j-1 {
				out[i], out[j] = out[j], out[i]
			}
			return out, nil
		}
		for _, next := range adj[n] {
			if !seen[next] {
				seen[next] = true
				parent[next] = n
				queue = append(queue, next)
			}
		}
	}
	return []string{}, nil
}

func main() {}

func RemediationCut(raw, start, goal string) ([]Edge, error) {
	if _, err := AdminPath(raw, start, goal); err != nil {
		return nil, err
	}
	var d Directory
	if err := json.Unmarshal([]byte(raw), &d); err != nil {
		return nil, err
	}
	candidates := []Edge{}
	unique := map[Edge]bool{}
	for _, e := range d.Edges {
		switch e.Kind {
		case "GenericAll", "AdminTo", "HasSession":
			if !unique[e] {
				unique[e] = true
				candidates = append(candidates, e)
			}
		}
	}
	if len(candidates) > 16 {
		return nil, fmt.Errorf("too many removable relationships")
	}
	sort.Slice(candidates, func(i, j int) bool {
		a, b := candidates[i], candidates[j]
		if a.From != b.From {
			return a.From < b.From
		}
		if a.To != b.To {
			return a.To < b.To
		}
		return a.Kind < b.Kind
	})
	reachable := func(removed map[Edge]bool) bool {
		adj := map[string][]string{}
		for _, e := range d.Edges {
			if removed[e] {
				continue
			}
			switch e.Kind {
			case "MemberOf", "GenericAll", "AdminTo", "HasSession":
				adj[e.From] = append(adj[e.From], e.To)
			}
		}
		q := []string{start}
		seen := map[string]bool{start: true}
		for len(q) > 0 {
			n := q[0]
			q = q[1:]
			if n == goal {
				return true
			}
			for _, to := range adj[n] {
				if !seen[to] {
					seen[to] = true
					q = append(q, to)
				}
			}
		}
		return false
	}
	removed := map[Edge]bool{}
	if !reachable(removed) {
		return []Edge{}, nil
	}
	for size := 1; size <= len(candidates); size++ {
		chosen := []Edge{}
		var answer []Edge
		var search func(int, int) bool
		search = func(from, left int) bool {
			if left == 0 {
				if !reachable(removed) {
					answer = append([]Edge{}, chosen...)
					return true
				}
				return false
			}
			for i := from; i <= len(candidates)-left; i++ {
				e := candidates[i]
				removed[e] = true
				chosen = append(chosen, e)
				if search(i+1, left-1) {
					return true
				}
				chosen = chosen[:len(chosen)-1]
				delete(removed, e)
			}
			return false
		}
		if search(0, size) {
			return answer, nil
		}
	}
	return nil, fmt.Errorf("no removable cut")
}
