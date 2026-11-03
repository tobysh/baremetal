package main

import (
	"errors"
	"math/rand"
	"reflect"
	"testing"
)

type failingDisk struct {
	p      Persistent
	fail   bool
	writes int
}

func (d *failingDisk) Load() Persistent {
	return Persistent{d.p.Term, d.p.VotedFor, append([]Entry(nil), d.p.Log...)}
}
func (d *failingDisk) Save(p Persistent) error {
	d.writes++
	if d.fail {
		return errors.New("power loss")
	}
	d.p = Persistent{p.Term, p.VotedFor, append([]Entry(nil), p.Log...)}
	return nil
}
func TestDurability(t *testing.T) {
	r := rand.New(rand.NewSource(81))
	d := &failingDisk{}
	s := NewServer(d)
	for step := 0; step < 2000; step++ {
		before := d.Load()
		model := &Node{Term: before.Term, VotedFor: before.VotedFor, Log: append([]Entry(nil), before.Log...), Commit: s.Node().Commit}
		d.fail = r.Intn(5) == 0
		writes := d.writes
		var err error
		var got, want any
		if r.Intn(2) == 0 {
			q := VoteReq{Term: r.Intn(30), Candidate: 1 + r.Intn(3), LastIndex: r.Intn(8), LastTerm: r.Intn(30)}
			want = model.HandleVote(q)
			got, err = s.Vote(q)
		} else {
			q := AppendReq{Term: r.Intn(30), Leader: 1, PrevIndex: 0, Entries: []Entry{{Term: r.Intn(30), Cmd: "set x"}}, LeaderCommit: r.Intn(8)}
			want = model.HandleAppend(q)
			got, err = s.Append(q)
		}
		after := Persistent{model.Term, model.VotedFor, model.Log}
		changed := !equalDurable(before, after)
		if changed && d.fail {
			if err == nil {
				t.Fatal("failed save acknowledged")
			}
			if !equalDurable(durableState(s), before) {
				t.Fatal("memory ahead of disk")
			}
		} else {
			if err != nil || !reflect.DeepEqual(got, want) || !equalDurable(d.Load(), after) {
				t.Fatal("reply or state", step, err)
			}
		}
		expected := 0
		if changed {
			expected = 1
		}
		if d.writes-writes != expected {
			t.Fatal("save count")
		}
		if r.Intn(3) == 0 {
			s = NewServer(d)
			if s.Node().Commit != 0 || !equalDurable(durableState(s), d.Load()) {
				t.Fatal("restart")
			}
		}
	}
}
func TestVoteSurvivesRestart(t *testing.T) {
	d := &MemDisk{}
	s := NewServer(d)
	v, e := s.Vote(VoteReq{Term: 7, Candidate: 1})
	if e != nil || !v.Granted {
		t.Fatal(v, e)
	}
	s = NewServer(d)
	v, e = s.Vote(VoteReq{Term: 7, Candidate: 2})
	if e != nil || v.Granted {
		t.Fatal("double vote")
	}
}

func durableState(s *Server) Persistent { n := s.Node(); return Persistent{n.Term, n.VotedFor, n.Log} }
func equalDurable(a, b Persistent) bool {
	if a.Term != b.Term || a.VotedFor != b.VotedFor || len(a.Log) != len(b.Log) {
		return false
	}
	for i := range a.Log {
		if a.Log[i] != b.Log[i] {
			return false
		}
	}
	return true
}
