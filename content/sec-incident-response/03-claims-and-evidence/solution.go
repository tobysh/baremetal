package main

type Evidence struct {
	ID, Source, Subject, Attr, Value string
	Observed                         bool
}

type Claim struct {
	ID, Subject, Attr, Value string
	Cites, Assumes           []string
}

type Verdict struct {
	ClaimID, Status string
}

func Assess(ev []Evidence, claims []Claim) []Verdict {
	byID := map[string]Evidence{}
	for _, e := range ev {
		byID[e.ID] = e
	}
	out := make([]Verdict, 0, len(claims))
	for _, c := range claims {
		out = append(out, Verdict{c.ID, status(ev, byID, c)})
	}
	return out
}

func status(ev []Evidence, byID map[string]Evidence, c Claim) string {
	if len(c.Cites) == 0 {
		return "unsupported"
	}
	for _, id := range c.Cites {
		if _, ok := byID[id]; !ok {
			return "unsupported"
		}
	}
	for _, e := range ev {
		if e.Observed && e.Subject == c.Subject && e.Attr == c.Attr && e.Value != c.Value {
			return "contradicted"
		}
	}
	reported := false
	for _, id := range c.Cites {
		e := byID[id]
		if e.Subject == c.Subject && e.Attr == c.Attr && e.Value == c.Value {
			if e.Observed {
				return "supported"
			}
			reported = true
		}
	}
	if reported {
		return "reported"
	}
	if len(c.Assumes) > 0 {
		return "inferred"
	}
	return "unsupported"
}

func main() {}
