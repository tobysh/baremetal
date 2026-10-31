package main

type Finding struct {
	ID, Kind, Ref string
	Version       int
}

type Snapshot struct {
	Time        int64
	Covers      map[string]bool
	Accounts    []string
	Keys        []string
	Files       []string
	CredVersion map[string]int
	OldRejected map[string]bool
}

type Activity struct {
	Time int64
	IP   string
}

type Result struct{ ID, Status string }

func Verify(findings []Finding, snap Snapshot, fixedAt int64, iocIPs []string, activity []Activity) ([]Result, bool) {
	return nil, false
}

func main() {}
