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
