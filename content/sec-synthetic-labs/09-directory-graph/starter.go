package main

type Node struct{ ID, Type, Label string }
type Edge struct{ From, To, Kind string }
type Directory struct {
	Nodes []Node
	Edges []Edge
}

func AdminPath(raw, start, goal string) ([]string, error) {
	return nil, nil
}

func main() {}
