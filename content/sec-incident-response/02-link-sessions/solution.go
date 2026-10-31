package main

type Login struct {
	Time    int64
	User    string
	IP      string
	OK      bool
	Session string
}

type Request struct {
	Time    int64
	IP      string
	Session string
	Path    string
}

type Link struct {
	Index int
	User  string
	Basis string
}

func Attribute(logins []Login, reqs []Request, window int64) []Link {
	out := make([]Link, len(reqs))
	for i, r := range reqs {
		out[i] = Link{Index: i, Basis: "none"}
		if r.Session != "" {
			out[i].Basis = "unknown-session"
			for _, l := range logins {
				if l.OK && l.Session == r.Session && l.Time <= r.Time {
					out[i].User, out[i].Basis = l.User, "session"
					break
				}
			}
			continue
		}
		users := map[string]bool{}
		last := ""
		for _, l := range logins {
			if l.OK && l.IP == r.IP && l.Time >= r.Time-window && l.Time <= r.Time {
				users[l.User] = true
				last = l.User
			}
		}
		switch len(users) {
		case 0:
		case 1:
			out[i].User, out[i].Basis = last, "ip-time"
		default:
			out[i].Basis = "ambiguous"
		}
	}
	return out
}

func main() {}
