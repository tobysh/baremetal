package main

import "fmt"

import (
	"encoding/json"
	"errors"
)

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
	next, match []int
}

func NewLeader(id, n, term int, log []Entry) *Leader {
	l := &Leader{ID: id, N: n, Term: term, Log: append([]Entry(nil), log...)}
	l.next = make([]int, n)
	l.match = make([]int, n)
	for i := range l.next {
		l.next[i] = len(l.Log) + 1
	}
	return l
}

func (l *Leader) Propose(cmd string) int {
	l.Log = append(l.Log, Entry{l.Term, cmd})
	l.advance()
	return len(l.Log)
}

func (l *Leader) Request(peer, max int) AppendReq {
	prev := l.next[peer] - 1
	pt := 0
	if prev > 0 {
		pt = l.Log[prev-1].Term
	}
	end := len(l.Log)
	if max > 0 && prev+max < end {
		end = prev + max
	}
	return AppendReq{Term: l.Term, Leader: l.ID, PrevIndex: prev, PrevTerm: pt,
		Entries: append([]Entry(nil), l.Log[prev:end]...), LeaderCommit: l.Commit}
}

func (l *Leader) OnReply(peer int, req AppendReq, resp AppendResp) {
	if l.Deposed {
		return
	}
	if resp.Term > l.Term {
		l.Deposed = true
		return
	}
	if resp.Success {
		if m := req.PrevIndex + len(req.Entries); m > l.match[peer] {
			l.match[peer] = m
		}
	} else if n := resp.Match + 1; n < l.next[peer] {
		l.next[peer] = n
	}
	if l.next[peer] < l.match[peer]+1 {
		l.next[peer] = l.match[peer] + 1
	}
	l.advance()
}

func (l *Leader) advance() {
	for i := len(l.Log); i > l.Commit; i-- {
		if l.Log[i-1].Term != l.Term {
			continue
		}
		count := 1
		for p := 0; p < l.N; p++ {
			if p != l.ID && l.match[p] >= i {
				count++
			}
		}
		if count >= l.N/2+1 {
			l.Commit = i
			return
		}
	}
}

// A stable request ID identifies a retry. The first committed command wins.
type Command struct{ ID, Key, Value string }
type Store struct {
	Values  map[string]string
	Seen    map[string]bool
	Applied int
}

func NewStore() *Store        { return &Store{Values: map[string]string{}, Seen: map[string]bool{}} }
func Encode(c Command) string { b, _ := json.Marshal(c); return string(b) }

var ErrCommit = errors.New("invalid committed prefix")

func (s *Store) Apply(log []Entry, commit int) error {
	if commit < s.Applied || commit > len(log) {
		return ErrCommit
	}
	commands := make([]Command, commit-s.Applied)
	for i := s.Applied; i < commit; i++ {
		var c Command
		if json.Unmarshal([]byte(log[i].Cmd), &c) != nil || c.ID == "" || c.Key == "" {
			return ErrCommit
		}
		commands[i-s.Applied] = c
	}
	for _, c := range commands {
		if !s.Seen[c.ID] {
			s.Values[c.Key] = c.Value
			s.Seen[c.ID] = true
		}
	}
	s.Applied = commit
	return nil
}
func main() {
	s := NewStore()
	log := []Entry{{Term: 1, Cmd: Encode(Command{ID: "request-1", Key: "x", Value: "hello"})}}
	fmt.Println(s.Apply(log, 1), s.Values["x"])
}
