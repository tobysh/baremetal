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
	return AppendResp{}
}

// Try it: accept two entries, then see a request that skips ahead rejected.
func main() {
	f := NewFollower()
	resp := f.HandleAppend(AppendReq{Term: 1, Entries: []Entry{{1, "a"}, {1, "b"}}, LeaderCommit: 1})
	fmt.Println(resp, f.Log, f.Commit)
	resp = f.HandleAppend(AppendReq{Term: 1, PrevIndex: 5, PrevTerm: 1})
	fmt.Println(resp) // rejected: the follower has no entry 5
}
