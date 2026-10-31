package main

import "fmt"

type Entry struct {
	Term int
	Cmd  string
}

type VoteReq struct {
	Term, Candidate     int
	LastIndex, LastTerm int
}

type VoteResp struct {
	Term    int
	Granted bool
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

// Persistent is what must survive a crash.
type Persistent struct {
	Term     int
	VotedFor int // 0 means nobody; node ids are 1, 2, 3, ...
	Log      []Entry
}

// Disk stores one Persistent value atomically: Save either completes or
// leaves the old value in place. A fresh disk loads the zero Persistent.
type Disk interface {
	Save(p Persistent) error
	Load() Persistent
}

// MemDisk is a simple in-memory Disk.
type MemDisk struct{ p Persistent }

func (d *MemDisk) Save(p Persistent) error {
	d.p = Persistent{p.Term, p.VotedFor, append([]Entry(nil), p.Log...)}
	return nil
}

func (d *MemDisk) Load() Persistent {
	return Persistent{d.p.Term, d.p.VotedFor, append([]Entry(nil), d.p.Log...)}
}

// Node is the in-memory protocol logic (supplied). Commit is volatile.
type Node struct {
	Term     int
	VotedFor int
	Log      []Entry
	Commit   int
}

func (n *Node) HandleVote(r VoteReq) VoteResp {
	if r.Term > n.Term {
		n.Term, n.VotedFor = r.Term, 0
	}
	lastTerm := 0
	if len(n.Log) > 0 {
		lastTerm = n.Log[len(n.Log)-1].Term
	}
	upToDate := r.LastTerm > lastTerm || (r.LastTerm == lastTerm && r.LastIndex >= len(n.Log))
	grant := r.Term == n.Term && (n.VotedFor == 0 || n.VotedFor == r.Candidate) && upToDate
	if grant {
		n.VotedFor = r.Candidate
	}
	return VoteResp{Term: n.Term, Granted: grant}
}

func (n *Node) HandleAppend(r AppendReq) AppendResp {
	if r.Term < n.Term {
		return AppendResp{Term: n.Term}
	}
	if r.Term > n.Term {
		n.Term, n.VotedFor = r.Term, 0
	}
	if r.PrevIndex > len(n.Log) || (r.PrevIndex > 0 && n.Log[r.PrevIndex-1].Term != r.PrevTerm) {
		hint := r.PrevIndex - 1
		if len(n.Log) < hint {
			hint = len(n.Log)
		}
		return AppendResp{Term: n.Term, Match: hint}
	}
	for i, e := range r.Entries {
		idx := r.PrevIndex + i
		if idx < len(n.Log) {
			if n.Log[idx].Term == e.Term {
				continue
			}
			n.Log = n.Log[:idx]
		}
		n.Log = append(n.Log, e)
	}
	last := r.PrevIndex + len(r.Entries)
	if r.LeaderCommit > n.Commit {
		c := min(r.LeaderCommit, last)
		if c > n.Commit {
			n.Commit = c
		}
	}
	return AppendResp{Term: n.Term, Success: true, Match: last}
}

type Server struct {
	disk Disk
	node *Node
}

// NewServer builds a server from whatever the disk holds.
func NewServer(d Disk) *Server {
	return &Server{disk: d, node: &Node{}}
}

func (s *Server) Node() *Node { return s.node }

// Vote handles a vote request. Persist before replying.
func (s *Server) Vote(r VoteReq) (VoteResp, error) {
	return s.node.HandleVote(r), nil
}

// Append handles an AppendEntries request. Persist before replying.
func (s *Server) Append(r AppendReq) (AppendResp, error) {
	return s.node.HandleAppend(r), nil
}

// Try it: vote, crash, and see whether the vote was remembered.
func main() {
	disk := &MemDisk{}
	s := NewServer(disk)
	resp, _ := s.Vote(VoteReq{Term: 5, Candidate: 1})
	fmt.Println("first vote:", resp)
	s = NewServer(disk) // crash and restart
	resp, _ = s.Vote(VoteReq{Term: 5, Candidate: 2})
	fmt.Println("after restart, other candidate:", resp) // must be refused
}
