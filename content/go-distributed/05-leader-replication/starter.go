package main

import "fmt"

type Entry struct {
	Term int
	Cmd  string
}

type AppendReq struct {
	Term         int
	Leader       int
	PrevIndex    int
	PrevTerm     int
	Entries      []Entry
	LeaderCommit int
}

type AppendResp struct {
	Term    int
	Success bool
	Match   int
}

// Follower is supplied (it is the previous lesson's follower).
type Follower struct {
	Term   int
	Log    []Entry // Log[i] is the entry at index i+1
	Commit int
}

func NewFollower() *Follower { return &Follower{} }

func (f *Follower) HandleAppend(r AppendReq) AppendResp {
	if r.Term < f.Term {
		return AppendResp{Term: f.Term}
	}
	f.Term = r.Term
	if r.PrevIndex > len(f.Log) || (r.PrevIndex > 0 && f.Log[r.PrevIndex-1].Term != r.PrevTerm) {
		hint := r.PrevIndex - 1
		if len(f.Log) < hint {
			hint = len(f.Log)
		}
		return AppendResp{Term: f.Term, Match: hint}
	}
	for i, e := range r.Entries {
		idx := r.PrevIndex + i
		if idx < len(f.Log) {
			if f.Log[idx].Term == e.Term {
				continue
			}
			f.Log = f.Log[:idx]
		}
		f.Log = append(f.Log, e)
	}
	last := r.PrevIndex + len(r.Entries)
	if r.LeaderCommit > f.Commit {
		c := r.LeaderCommit
		if last < c {
			c = last
		}
		if c > f.Commit {
			f.Commit = c
		}
	}
	return AppendResp{Term: f.Term, Success: true, Match: last}
}

type Leader struct {
	ID, N, Term int
	Log         []Entry
	Commit      int
	Deposed     bool
	// add next and match tracking here
}

func NewLeader(id, n, term int, log []Entry) *Leader {
	l := &Leader{ID: id, N: n, Term: term, Log: append([]Entry(nil), log...)}
	// initialise next and match here
	return l
}

// Propose appends a client command in the leader's term and returns its index.
func (l *Leader) Propose(cmd string) int {
	return 0
}

// Request builds the AppendEntries request for peer, with at most max
// entries (all remaining entries when max <= 0).
func (l *Leader) Request(peer, max int) AppendReq {
	return AppendReq{}
}

// OnReply processes the follower's reply to req. Replies may be late,
// duplicated or out of order.
func (l *Leader) OnReply(peer int, req AppendReq, resp AppendResp) {}

// Try it: a leader and two followers on a reliable network.
func main() {
	l := NewLeader(0, 3, 1, nil)
	fs := []*Follower{nil, NewFollower(), NewFollower()}
	l.Propose("x")
	l.Propose("y")
	for p := 1; p <= 2; p++ {
		req := l.Request(p, 0)
		l.OnReply(p, req, fs[p].HandleAppend(req))
	}
	fmt.Println("leader commit:", l.Commit) // 2
}
