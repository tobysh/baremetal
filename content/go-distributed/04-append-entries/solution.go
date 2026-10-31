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

func main() {
	f := NewFollower()
	resp := f.HandleAppend(AppendReq{Term: 1, Entries: []Entry{{1, "a"}, {1, "b"}}, LeaderCommit: 1})
	fmt.Println(resp, f.Log, f.Commit)
	resp = f.HandleAppend(AppendReq{Term: 1, PrevIndex: 5, PrevTerm: 1})
	fmt.Println(resp) // rejected: the follower has no entry 5
}
