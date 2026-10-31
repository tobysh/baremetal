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

func has(list []string, s string) bool {
	for _, x := range list {
		if x == s {
			return true
		}
	}
	return false
}

func Verify(findings []Finding, snap Snapshot, fixedAt int64, iocIPs []string, activity []Activity) ([]Result, bool) {
	res := make([]Result, 0, len(findings))
	for _, f := range findings {
		st := "open"
		switch {
		case snap.Time < fixedAt || !snap.Covers[f.Kind]:
			st = "unverifiable"
		case f.Kind == "credential":
			v, ok := snap.CredVersion[f.Ref]
			if !ok {
				st = "unverifiable"
			} else if v > f.Version && snap.OldRejected[f.Ref] {
				st = "closed"
			}
		default:
			var l []string
			switch f.Kind {
			case "account":
				l = snap.Accounts
			case "key":
				l = snap.Keys
			case "file":
				l = snap.Files
			}
			if !has(l, f.Ref) {
				st = "closed"
			}
		}
		res = append(res, Result{f.ID, st})
	}
	rec := false
	for _, a := range activity {
		if a.Time > fixedAt && has(iocIPs, a.IP) {
			rec = true
		}
	}
	return res, rec
}

func main() {}
