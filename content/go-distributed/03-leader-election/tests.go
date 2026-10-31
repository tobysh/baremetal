package main

import (
	"math/rand"
	"testing"
)

func cluster(n int) []*Node {
	ns := make([]*Node, n)
	for i := range ns {
		ns[i] = NewNode(i, n)
	}
	return ns
}

// elect runs a fully reliable election for candidate c.
func elect(ns []*Node, c int) {
	req := ns[c].StartElection()
	for i, o := range ns {
		if i != c {
			ns[c].HandleVoteResp(o.HandleVoteReq(req))
		}
	}
}

func TestReliableElection(t *testing.T) {
	ns := cluster(5)
	elect(ns, 2)
	if ns[2].Role != Leader || ns[2].Term != 1 {
		t.Fatalf("node 2 after a clean election: role %v term %d; want Leader, term 1", ns[2].Role, ns[2].Term)
	}
	for i, o := range ns {
		if i != 2 && (o.VotedFor != 2 || o.Term != 1 || o.Role != Follower) {
			t.Fatalf("node %d: term %d votedFor %d role %v; want term 1, votedFor 2, Follower", i, o.Term, o.VotedFor, o.Role)
		}
	}
	// a later election by someone else takes over and the old leader steps down
	elect(ns, 4)
	if ns[4].Role != Leader || ns[4].Term != 2 || ns[2].Role != Follower || ns[2].Term != 2 {
		t.Fatalf("after node 4's election: node4 %v/%d node2 %v/%d; want Leader/2 and Follower/2", ns[4].Role, ns[4].Term, ns[2].Role, ns[2].Term)
	}
}

func TestSingleNodeClusterElectsItself(t *testing.T) {
	n := NewNode(0, 1)
	n.StartElection()
	if n.Role != Leader || n.Term != 1 {
		t.Fatalf("one-node cluster: role %v term %d; want Leader, term 1", n.Role, n.Term)
	}
}

func TestOneVotePerTerm(t *testing.T) {
	v := NewNode(2, 3)
	a := v.HandleVoteReq(VoteReq{Term: 1, Candidate: 0})
	b := v.HandleVoteReq(VoteReq{Term: 1, Candidate: 1})
	again := v.HandleVoteReq(VoteReq{Term: 1, Candidate: 0})
	if !a.Granted || b.Granted {
		t.Fatalf("first request granted=%v second (other candidate, same term) granted=%v; want true, false", a.Granted, b.Granted)
	}
	if !again.Granted {
		t.Fatalf("a repeated request from the candidate we already voted for must be granted again (the reply may have been lost)")
	}
	c := v.HandleVoteReq(VoteReq{Term: 2, Candidate: 1})
	if !c.Granted || c.Term != 2 || v.VotedFor != 1 {
		t.Fatalf("a higher term resets the vote: got granted=%v term=%d votedFor=%d; want true, 2, 1", c.Granted, c.Term, v.VotedFor)
	}
}

func TestStaleVoteRequestRejected(t *testing.T) {
	v := NewNode(1, 3)
	v.HandleVoteReq(VoteReq{Term: 5, Candidate: 0})
	r := v.HandleVoteReq(VoteReq{Term: 3, Candidate: 2})
	if r.Granted || r.Term != 5 || v.Term != 5 {
		t.Fatalf("stale request: granted=%v replyTerm=%d nodeTerm=%d; want false, 5, 5", r.Granted, r.Term, v.Term)
	}
}

func TestLogUpToDateRule(t *testing.T) {
	cases := []struct {
		vIdx, vTerm, cIdx, cTerm int
		grant                    bool
	}{
		{0, 0, 0, 0, true},
		{3, 1, 3, 1, true},
		{3, 1, 4, 1, true},
		{3, 1, 2, 1, false},
		{3, 1, 1, 2, true},  // a later term beats a longer log
		{3, 2, 9, 1, false}, // a longer log of older entries does not
		{5, 3, 5, 2, false},
	}
	for _, c := range cases {
		v := NewNode(1, 3)
		v.LastIndex, v.LastTerm = c.vIdx, c.vTerm
		r := v.HandleVoteReq(VoteReq{Term: 1, Candidate: 0, LastIndex: c.cIdx, LastTerm: c.cTerm})
		if r.Granted != c.grant {
			t.Errorf("voter log (idx %d, term %d), candidate log (idx %d, term %d): granted=%v; want %v", c.vIdx, c.vTerm, c.cIdx, c.cTerm, r.Granted, c.grant)
		}
	}
	// even when refusing, the voter adopts the higher term
	v := NewNode(1, 3)
	v.LastIndex, v.LastTerm = 4, 2
	v.HandleVoteReq(VoteReq{Term: 7, Candidate: 0, LastIndex: 1, LastTerm: 1})
	if v.Term != 7 || v.VotedFor != -1 {
		t.Errorf("after refusing a behind candidate in term 7: term %d votedFor %d; want 7, -1", v.Term, v.VotedFor)
	}
}

func TestDuplicateAndStaleVotesDoNotCount(t *testing.T) {
	n := NewNode(0, 5)
	n.StartElection()
	g := VoteResp{Term: 1, From: 1, Granted: true}
	n.HandleVoteResp(g)
	n.HandleVoteResp(g)
	n.HandleVoteResp(g)
	if n.Role == Leader {
		t.Fatalf("became leader of 5 on its own vote plus one voter's duplicated reply")
	}
	n.HandleVoteResp(VoteResp{Term: 0, From: 2, Granted: true}) // from an older term
	n.HandleVoteResp(VoteResp{Term: 1, From: 3, Granted: false})
	if n.Role == Leader {
		t.Fatalf("became leader counting a stale-term or refused reply")
	}
	n.HandleVoteResp(VoteResp{Term: 1, From: 2, Granted: true})
	if n.Role != Leader {
		t.Fatalf("not a leader with 3 distinct votes of 5")
	}
}

func TestLateVoteAfterNewElectionIgnored(t *testing.T) {
	n := NewNode(0, 3)
	n.StartElection() // term 1
	n.StartElection() // term 2, the term-1 replies are now stale
	n.HandleVoteResp(VoteResp{Term: 1, From: 1, Granted: true})
	if n.Role == Leader {
		t.Fatalf("a term-1 vote made the term-2 candidate a leader")
	}
	n.HandleVoteResp(VoteResp{Term: 2, From: 1, Granted: true})
	if n.Role != Leader {
		t.Fatalf("not a leader with a term-2 vote from node 1 plus itself in a cluster of 3")
	}
}

func TestHigherTermStepsDown(t *testing.T) {
	n := NewNode(0, 3)
	n.StartElection()
	n.HandleVoteResp(VoteResp{Term: 4, From: 1, Granted: false})
	if n.Role != Follower || n.Term != 4 || n.VotedFor != -1 {
		t.Fatalf("after a reply with term 4: role %v term %d votedFor %d; want Follower, 4, -1", n.Role, n.Term, n.VotedFor)
	}
	l := NewNode(1, 3)
	elect([]*Node{NewNode(0, 3), l, NewNode(2, 3)}, 1)
	if l.Role != Leader {
		t.Fatal("setup: no leader")
	}
	l.HandleVoteReq(VoteReq{Term: l.Term + 1, Candidate: 2})
	if l.Role != Follower {
		t.Fatalf("a leader that sees a vote request with a higher term must step down")
	}
}

func TestHeartbeat(t *testing.T) {
	n := NewNode(0, 3)
	n.StartElection() // candidate, term 1
	if n.HandleHeartbeat(0, 2) {
		t.Fatalf("heartbeat from term 0 accepted by a node in term 1")
	}
	if n.Role != Candidate {
		t.Fatalf("a stale heartbeat changed the role to %v", n.Role)
	}
	if !n.HandleHeartbeat(1, 2) || n.Role != Follower || n.Term != 1 || n.VotedFor != 0 {
		t.Fatalf("same-term heartbeat: role %v term %d votedFor %d; want Follower, 1, 0 (the vote is kept)", n.Role, n.Term, n.VotedFor)
	}
	if !n.HandleHeartbeat(6, 1) || n.Term != 6 || n.VotedFor != -1 {
		t.Fatalf("higher-term heartbeat: term %d votedFor %d; want 6, -1", n.Term, n.VotedFor)
	}
}

type msg struct {
	to, from int
	kind     int // 0 vote request, 1 vote reply, 2 heartbeat
	req      VoteReq
	resp     VoteResp
	term     int
}

func TestRandomSchedules(t *testing.T) {
	for seed := int64(1); seed <= 160; seed++ {
		runElectionSchedule(t, seed)
		if t.Failed() {
			return
		}
	}
}

func runElectionSchedule(t *testing.T, seed int64) {
	rng := rand.New(rand.NewSource(seed))
	n := 3 + 2*int(seed%2)
	ns := cluster(n)
	behind := map[int]bool{}
	if seed%3 != 0 && n == 5 {
		// nodes 0..2 hold a committed entry (index 5, term 2); 3 and 4 are behind.
		for i := 0; i < 3; i++ {
			ns[i].LastIndex, ns[i].LastTerm = 5, 2
		}
		for i := 3; i < 5; i++ {
			ns[i].LastIndex, ns[i].LastTerm = 3+int(seed%2), 1
			behind[i] = true
		}
	}
	group := make([]int, n)
	var pool []msg
	leaders := map[int]int{}             // term -> leader id
	granted := map[[2]int]map[int]bool{} // (candidate, term) -> voters
	voted := map[[2]int]int{}            // (voter, term) -> candidate
	lastTerm := make([]int, n)
	send := func(m msg) { pool = append(pool, m) }
	for step := 0; step < 1500; step++ {
		switch r := rng.Intn(100); {
		case r < 8: // election timeout
			i := rng.Intn(n)
			req := ns[i].StartElection()
			key := [2]int{i, req.Term}
			granted[key] = map[int]bool{i: true}
			for j := 0; j < n; j++ {
				if j != i {
					send(msg{to: j, from: i, kind: 0, req: req})
				}
			}
		case r < 16: // heartbeats from every leader
			for i, nd := range ns {
				if nd.Role == Leader {
					for j := 0; j < n; j++ {
						if j != i {
							send(msg{to: j, from: i, kind: 2, term: nd.Term})
						}
					}
				}
			}
		case r < 19: // re-partition
			k := 1 + rng.Intn(3)
			for i := range group {
				group[i] = rng.Intn(k)
			}
		default:
			if len(pool) == 0 {
				continue
			}
			i := rng.Intn(len(pool))
			m := pool[i]
			if rng.Intn(10) != 0 {
				pool[i] = pool[len(pool)-1]
				pool = pool[:len(pool)-1]
			} // else it stays in flight and is delivered again later
			if group[m.to] != group[m.from] || rng.Intn(6) == 0 {
				continue // partitioned or dropped
			}
			nd := ns[m.to]
			switch m.kind {
			case 0:
				before := nd.Term
				resp := nd.HandleVoteReq(m.req)
				if resp.From != m.to || resp.Term != nd.Term {
					t.Fatalf("seed %d: reply %+v from node %d whose term is %d", seed, resp, m.to, nd.Term)
				}
				if resp.Granted {
					if m.req.Term != nd.Term {
						t.Fatalf("seed %d: node %d granted a vote for term %d while in term %d", seed, m.to, m.req.Term, nd.Term)
					}
					k := [2]int{m.to, m.req.Term}
					if prev, ok := voted[k]; ok && prev != m.req.Candidate {
						t.Fatalf("seed %d: node %d voted for both %d and %d in term %d", seed, m.to, prev, m.req.Candidate, m.req.Term)
					}
					voted[k] = m.req.Candidate
					ck := [2]int{m.req.Candidate, m.req.Term}
					if granted[ck] == nil {
						granted[ck] = map[int]bool{m.req.Candidate: true}
					}
					granted[ck][m.to] = true
				} else if m.req.Term > before && nd.Term < m.req.Term {
					t.Fatalf("seed %d: node %d ignored the higher term %d (still in %d)", seed, m.to, m.req.Term, nd.Term)
				}
				send(msg{to: m.from, from: m.to, kind: 1, resp: resp})
			case 1:
				nd.HandleVoteResp(m.resp)
			case 2:
				before := nd.Term
				ok := nd.HandleHeartbeat(m.term, m.from)
				if ok != (m.term >= before) {
					t.Fatalf("seed %d: heartbeat term %d to a node in term %d returned %v", seed, m.term, before, ok)
				}
			}
		}
		// invariants after every step
		for i, nd := range ns {
			if nd.Term < lastTerm[i] {
				t.Fatalf("seed %d: node %d went from term %d back to %d", seed, i, lastTerm[i], nd.Term)
			}
			lastTerm[i] = nd.Term
			if nd.Role == Leader {
				if prev, ok := leaders[nd.Term]; ok && prev != i {
					t.Fatalf("seed %d: nodes %d and %d are both leader of term %d", seed, prev, i, nd.Term)
				}
				leaders[nd.Term] = i
				if got := len(granted[[2]int{i, nd.Term}]); got < n/2+1 {
					t.Fatalf("seed %d: node %d is leader of term %d with only %d genuine votes of %d", seed, i, nd.Term, got, n)
				}
				if behind[i] {
					t.Fatalf("seed %d: node %d, whose log is behind a majority-held entry, won the election for term %d", seed, i, nd.Term)
				}
			}
			if nd.Role != Follower && nd.VotedFor != i {
				t.Fatalf("seed %d: node %d is %v but is not voting for itself (votedFor %d)", seed, i, nd.Role, nd.VotedFor)
			}
		}
	}
	if len(leaders) == 0 && seed%3 == 0 {
		t.Fatalf("seed %d: 1500 steps with random timeouts never produced a leader", seed)
	}
}
