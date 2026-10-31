package main

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

func Escalations(before, after []Edge, audit []Audit, target string) ([]string, []Enabler) {
	return nil, nil
}

func main() {}
