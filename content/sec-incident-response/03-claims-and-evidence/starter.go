package main

type Evidence struct {
	ID, Source, Subject, Attr, Value string
	Observed                         bool // recorded by a system or tool, not a statement by a person
}

type Claim struct {
	ID, Subject, Attr, Value string
	Cites, Assumes           []string
}

type Verdict struct {
	ClaimID, Status string
}

func Assess(ev []Evidence, claims []Claim) []Verdict {
	return nil
}

func main() {}
