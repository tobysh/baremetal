package main

import (
	"fmt"
	"math/rand"
	"reflect"
	"testing"
)

func entries(terms ...int) []Entry {
	out := make([]Entry, len(terms))
	for i, t := range terms {
		out[i] = Entry{t, fmt.Sprintf("old%d", i+1)}
	}
	return out
}

func ok(l *Leader, peer int, req AppendReq) AppendResp {
	return AppendResp{Term: l.Term, Success: true, Match: req.PrevIndex + len(req.Entries)}
}

func TestFreshLeaderSendsHeartbeat(t *testing.T) {
	l := NewLeader(0, 3, 3, entries(1, 2))
	r := l.Request(1, 0)
	if r.Term != 3 || r.Leader != 0 || r.PrevIndex != 2 || r.PrevTerm != 2 || len(r.Entries) != 0 || r.LeaderCommit != 0 {
		t.Fatalf("first request = %+v; want a heartbeat with PrevIndex 2, PrevTerm 2, Term 3", r)
	}
}

func TestBackoffUsesHintAndBatches(t *testing.T) {
	l := NewLeader(0, 3, 3, entries(1, 1, 2, 2, 2))
	r := l.Request(1, 0)
	l.OnReply(1, r, AppendResp{Term: 3, Success: false, Match: 1})
	r = l.Request(1, 2)
	if r.PrevIndex != 1 || r.PrevTerm != 1 || len(r.Entries) != 2 || r.Entries[0].Term != 1 {
		t.Fatalf("after hint Match=1: request %+v; want PrevIndex 1 and a batch of 2 entries starting at index 2", r)
	}
	l.OnReply(1, r, ok(l, 1, r))
	r = l.Request(1, 2)
	if r.PrevIndex != 3 || len(r.Entries) != 2 {
		t.Fatalf("after success up to 3: request %+v; want PrevIndex 3 and 2 entries", r)
	}
	l.OnReply(1, r, ok(l, 1, r))
	r = l.Request(1, 2)
	if r.PrevIndex != 5 || len(r.Entries) != 0 {
		t.Fatalf("fully caught up: request %+v; want a heartbeat at PrevIndex 5", r)
	}
}

func TestStaleRepliesDoNotRegress(t *testing.T) {
	l := NewLeader(0, 3, 2, entries(1, 1, 1, 1))
	r := l.Request(1, 0) // heartbeat at 4
	l.OnReply(1, r, AppendResp{Term: 2, Success: false, Match: 2})
	r2 := l.Request(1, 0) // PrevIndex 2, carries entries 3,4
	l.OnReply(1, r2, ok(l, 1, r2))
	// the failure of an even older request shows up again, late
	l.OnReply(1, r, AppendResp{Term: 2, Success: false, Match: 2})
	l.OnReply(1, r, AppendResp{Term: 2, Success: false, Match: 0})
	nr := l.Request(1, 0)
	if nr.PrevIndex != 4 {
		t.Fatalf("stale failures moved next back: PrevIndex = %d; the follower is already known to match up to 4", nr.PrevIndex)
	}
	// an old success must not lower match either
	small := AppendReq{Term: 2, PrevIndex: 0, Entries: entries(1)}
	l.OnReply(1, small, ok(l, 1, small))
	l.Propose("new")
	if c := l.Request(1, 0); c.PrevIndex != 4 {
		t.Fatalf("a stale success lowered the tracked match: PrevIndex = %d; want 4", c.PrevIndex)
	}
}

func TestOldTermEntriesNeedACurrentTermEntry(t *testing.T) {
	l := NewLeader(0, 3, 3, entries(1, 2))
	for p := 1; p <= 2; p++ {
		r := l.Request(p, 0)
		l.OnReply(p, r, ok(l, p, r))
	}
	if l.Commit != 0 {
		t.Fatalf("Commit = %d; entries from earlier terms are on every node but must not be committed by counting alone", l.Commit)
	}
	idx := l.Propose("mine")
	if idx != 3 || len(l.Log) != 3 || l.Log[2].Term != 3 {
		t.Fatalf("Propose returned %d, log %+v; want index 3 with term 3", idx, l.Log)
	}
	if l.Commit != 0 {
		t.Fatalf("Commit = %d before any follower stored the new entry", l.Commit)
	}
	r := l.Request(2, 0)
	l.OnReply(2, r, ok(l, 2, r))
	if l.Commit != 3 {
		t.Fatalf("Commit = %d; want 3 (the new entry is on the leader and one follower, 2 of 3, and carries the old ones with it)", l.Commit)
	}
}

func TestMajorityOfFive(t *testing.T) {
	l := NewLeader(2, 5, 1, nil)
	l.Propose("a")
	r := l.Request(0, 0)
	l.OnReply(0, r, ok(l, 0, r))
	if l.Commit != 0 {
		t.Fatalf("committed with the leader and one follower of five")
	}
	r = l.Request(4, 0)
	l.OnReply(4, r, ok(l, 4, r))
	if l.Commit != 1 {
		t.Fatalf("Commit = %d; the leader and two followers are 3 of 5", l.Commit)
	}
	l.OnReply(4, r, ok(l, 4, r)) // the same reply twice counts once
	l.Propose("b")
	l.OnReply(4, r, ok(l, 4, r))
	if l.Commit != 1 {
		t.Fatalf("Commit = %d after a duplicated old reply; entry 2 is only on the leader", l.Commit)
	}
}

func TestSingleNodeCommitsAtOnce(t *testing.T) {
	l := NewLeader(0, 1, 1, nil)
	l.Propose("x")
	if l.Commit != 1 {
		t.Fatalf("one-node cluster: Commit = %d; want 1", l.Commit)
	}
}

func TestHigherTermDeposes(t *testing.T) {
	l := NewLeader(0, 3, 2, nil)
	l.Propose("x")
	r := l.Request(1, 0)
	l.OnReply(1, r, AppendResp{Term: 5})
	if !l.Deposed {
		t.Fatalf("a reply with term 5 did not depose a term-2 leader")
	}
	l.OnReply(2, r, ok(l, 2, r))
	if l.Commit != 0 {
		t.Fatalf("a deposed leader advanced its commit index")
	}
}

func TestRequestEntriesAreCopies(t *testing.T) {
	l := NewLeader(0, 3, 1, entries(1, 1))
	l.OnReply(1, l.Request(1, 0), AppendResp{Term: 1, Match: 0})
	r := l.Request(1, 0)
	if len(r.Entries) == 0 {
		t.Fatalf("no entries to send to a follower that has none")
	}
	r.Entries[0] = Entry{99, "mutated"}
	if len(l.Log) == 0 || l.Log[0].Term != 1 {
		t.Fatalf("the request shares memory with the leader's log")
	}
}

func TestSchedules(t *testing.T) {
	for seed := int64(1); seed <= 150; seed++ {
		runLeaderSchedule(t, seed)
		if t.Failed() {
			return
		}
	}
}

type pending struct {
	peer  int
	req   AppendReq
	resp  AppendResp
	reply bool
}

func runLeaderSchedule(t *testing.T, seed int64) {
	rng := rand.New(rand.NewSource(seed))
	n := 3 + 2*int(seed%2)
	var old []Entry
	for i := rng.Intn(4); i > 0; i-- {
		old = append(old, Entry{1, fmt.Sprintf("a%d", i)})
	}
	for i := rng.Intn(4); i > 0; i-- {
		old = append(old, Entry{2, fmt.Sprintf("b%d", i)})
	}
	l := NewLeader(0, n, 4, old)
	fs := make([]*Follower, n)
	for p := 1; p < n; p++ {
		f := NewFollower()
		k := rng.Intn(len(old) + 1)
		f.Log = append(f.Log, old[:k]...)
		for d := rng.Intn(4); d > 0; d-- { // uncommitted entries from a failed term-3 leader
			f.Log = append(f.Log, Entry{3, fmt.Sprintf("junk%d", d)})
		}
		f.Term = 3
		fs[p] = f
	}
	var pool []pending
	proposed := 0
	lastCommit := 0
	check := func(step int) {
		if l.Commit < lastCommit {
			t.Fatalf("seed %d step %d: Commit went from %d back to %d", seed, step, lastCommit, l.Commit)
		}
		lastCommit = l.Commit
		if l.Commit > len(l.Log) {
			t.Fatalf("seed %d step %d: Commit %d beyond the leader's log (%d)", seed, step, l.Commit, len(l.Log))
		}
		if l.Commit > 0 && l.Log[l.Commit-1].Term != l.Term {
			t.Fatalf("seed %d step %d: Commit %d points at a term-%d entry; only the leader's own term (%d) may be committed directly", seed, step, l.Commit, l.Log[l.Commit-1].Term, l.Term)
		}
		for i := 1; i <= l.Commit; i++ {
			have := 1
			for p := 1; p < n; p++ {
				if len(fs[p].Log) >= i && fs[p].Log[i-1] == l.Log[i-1] {
					have++
				}
			}
			if have < n/2+1 {
				t.Fatalf("seed %d step %d: entry %d is committed but only %d of %d nodes store it", seed, step, i, have, n)
			}
		}
		if len(l.Log) < len(old) || !reflect.DeepEqual(append([]Entry(nil), l.Log[:len(old)]...), append([]Entry(nil), old...)) {
			t.Fatalf("seed %d step %d: the leader's inherited log changed", seed, step)
		}
	}
	for step := 0; step < 600; step++ {
		switch r := rng.Intn(100); {
		case r < 8 && proposed < 8:
			proposed++
			l.Propose(fmt.Sprintf("p%d", proposed))
		case r < 35:
			p := 1 + rng.Intn(n-1)
			req := l.Request(p, rng.Intn(4))
			if req.Term != 4 || req.PrevIndex < 0 || req.PrevIndex > len(l.Log) || req.PrevIndex+len(req.Entries) > len(l.Log) {
				t.Fatalf("seed %d step %d: malformed request %+v for a leader log of %d", seed, step, req, len(l.Log))
			}
			if req.PrevIndex > 0 && req.PrevTerm != l.Log[req.PrevIndex-1].Term {
				t.Fatalf("seed %d step %d: request %+v has the wrong PrevTerm (log says %d)", seed, step, req, l.Log[req.PrevIndex-1].Term)
			}
			if !reflect.DeepEqual(append([]Entry(nil), req.Entries...), append([]Entry(nil), l.Log[req.PrevIndex:req.PrevIndex+len(req.Entries)]...)) {
				t.Fatalf("seed %d step %d: request entries %v do not match the leader's log from index %d", seed, step, req.Entries, req.PrevIndex+1)
			}
			if req.LeaderCommit != l.Commit {
				t.Fatalf("seed %d step %d: LeaderCommit %d; leader's Commit is %d", seed, step, req.LeaderCommit, l.Commit)
			}
			if rng.Intn(100) >= 15 {
				pool = append(pool, pending{peer: p, req: req})
			}
		default:
			if len(pool) == 0 {
				continue
			}
			i := rng.Intn(len(pool))
			m := pool[i]
			if rng.Intn(10) > 0 {
				pool[i] = pool[len(pool)-1]
				pool = pool[:len(pool)-1]
			}
			if rng.Intn(100) < 15 {
				continue
			}
			if m.reply {
				l.OnReply(m.peer, m.req, m.resp)
			} else {
				q := m.req
				q.Entries = append([]Entry(nil), q.Entries...)
				pool = append(pool, pending{peer: m.peer, req: m.req, resp: fs[m.peer].HandleAppend(q), reply: true})
			}
		}
		check(step)
	}
	// the network heals: reliable rounds with small batches
	for round := 0; round < 400; round++ {
		for p := 1; p < n; p++ {
			req := l.Request(p, 2)
			q := req
			q.Entries = append([]Entry(nil), q.Entries...)
			l.OnReply(p, req, fs[p].HandleAppend(q))
		}
		check(1000 + round)
	}
	for p := 1; p < n; p++ {
		if !reflect.DeepEqual(append([]Entry(nil), fs[p].Log...), append([]Entry(nil), l.Log...)) {
			t.Fatalf("seed %d: follower %d log %v never converged to the leader's %v", seed, p, fs[p].Log, l.Log)
		}
	}
	if proposed == 0 {
		if l.Commit != 0 {
			t.Fatalf("seed %d: Commit = %d with no entry from the current term", seed, l.Commit)
		}
	} else if l.Commit != len(l.Log) {
		t.Fatalf("seed %d: after healing, Commit = %d; want %d", seed, l.Commit, len(l.Log))
	}
}
