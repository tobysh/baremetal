package main

import (
	"fmt"
	"math/rand"
	"reflect"
	"testing"
)

func es(terms ...int) []Entry {
	out := make([]Entry, len(terms))
	for i, t := range terms {
		out[i] = Entry{t, fmt.Sprintf("c%d.%d", i, t)}
	}
	return out
}

func sameEntries(a, b []Entry) bool {
	if len(a) != len(b) {
		return false
	}
	for i := range a {
		if a[i] != b[i] {
			return false
		}
	}
	return true
}

func logOf(f *Follower) []Entry { return append([]Entry{}, f.Log...) }

func TestAppendToEmpty(t *testing.T) {
	f := NewFollower()
	resp := f.HandleAppend(AppendReq{Term: 1, Leader: 0, Entries: es(1, 1, 2), LeaderCommit: 2})
	if !resp.Success || resp.Match != 3 || resp.Term != 1 {
		t.Fatalf("resp = %+v; want Success, Match 3, Term 1", resp)
	}
	if len(f.Log) != 3 || f.Commit != 2 || f.Term != 1 {
		t.Fatalf("state: log %v commit %d term %d; want 3 entries, commit 2, term 1", f.Log, f.Commit, f.Term)
	}
}

func TestHeartbeatKeepsLog(t *testing.T) {
	f := NewFollower()
	f.HandleAppend(AppendReq{Term: 1, Entries: es(1, 1, 1)})
	resp := f.HandleAppend(AppendReq{Term: 1, PrevIndex: 3, PrevTerm: 1, LeaderCommit: 3})
	if !resp.Success || resp.Match != 3 || f.Commit != 3 || len(f.Log) != 3 {
		t.Fatalf("heartbeat: resp %+v commit %d log len %d", resp, f.Commit, len(f.Log))
	}
}

func TestGapAndMismatchRejectedWithHint(t *testing.T) {
	f := NewFollower()
	f.HandleAppend(AppendReq{Term: 1, Entries: es(1, 1, 2)})
	before := logOf(f)
	r := f.HandleAppend(AppendReq{Term: 3, PrevIndex: 6, PrevTerm: 3, Entries: es(3)})
	if r.Success || r.Match != 3 || r.Term != 3 {
		t.Fatalf("gap: resp %+v; want failure with Match 3 (the follower's last index) and Term 3", r)
	}
	r = f.HandleAppend(AppendReq{Term: 3, PrevIndex: 3, PrevTerm: 3, Entries: es(3)})
	if r.Success || r.Match != 2 {
		t.Fatalf("term mismatch at index 3: resp %+v; want failure with Match 2", r)
	}
	r = f.HandleAppend(AppendReq{Term: 3, PrevIndex: 1, PrevTerm: 9})
	if r.Success || r.Match != 0 {
		t.Fatalf("term mismatch at index 1: resp %+v; want failure with Match 0", r)
	}
	if !reflect.DeepEqual(logOf(f), before) {
		t.Fatalf("a rejected request changed the log to %v", f.Log)
	}
	if f.Term != 3 {
		t.Fatalf("a rejected request from a newer term must still update the term; got %d", f.Term)
	}
}

func TestConflictTruncates(t *testing.T) {
	f := NewFollower()
	f.HandleAppend(AppendReq{Term: 1, Entries: es(1, 1, 1, 1)})
	// a new leader in term 2 overwrites from index 3
	r := f.HandleAppend(AppendReq{Term: 2, PrevIndex: 2, PrevTerm: 1, Entries: es(2, 2)})
	want := append(es(1, 1), es(2, 2)...)
	if !r.Success || r.Match != 4 || !reflect.DeepEqual(logOf(f), want) {
		t.Fatalf("resp %+v log %v; want Success, Match 4, log %v", r, f.Log, want)
	}
	// and a conflict further on than the first entry of the request
	f = NewFollower()
	f.HandleAppend(AppendReq{Term: 1, Entries: es(1, 1, 1, 1, 1)})
	req := append(es(1, 1), es(2)...) // entries 1,2 match; entry 3 conflicts
	f.HandleAppend(AppendReq{Term: 2, PrevIndex: 0, Entries: req})
	if len(f.Log) != 3 || f.Log[2].Term != 2 {
		t.Fatalf("conflict in the middle of a request: log %v; want 3 entries ending in term 2", f.Log)
	}
}

func TestShortDuplicateDoesNotTruncate(t *testing.T) {
	f := NewFollower()
	full := es(1, 1, 1, 1)
	f.HandleAppend(AppendReq{Term: 1, Entries: full})
	r := f.HandleAppend(AppendReq{Term: 1, Entries: full[:2]}) // an old, shorter copy arrives late
	if !r.Success || r.Match != 2 {
		t.Fatalf("resp %+v; want Success, Match 2", r)
	}
	if !reflect.DeepEqual(logOf(f), full) {
		t.Fatalf("a delayed shorter request cut the log down to %v; entries that match must be kept", f.Log)
	}
}

func TestStaleTermRejectedUntouched(t *testing.T) {
	f := NewFollower()
	f.HandleAppend(AppendReq{Term: 4, Entries: es(4, 4), LeaderCommit: 1})
	r := f.HandleAppend(AppendReq{Term: 3, Entries: es(3, 3, 3), LeaderCommit: 3})
	if r.Success || r.Term != 4 || len(f.Log) != 2 || f.Commit != 1 || f.Term != 4 {
		t.Fatalf("stale leader: resp %+v log len %d commit %d term %d; want rejection with Term 4 and no change", r, len(f.Log), f.Commit, f.Term)
	}
}

func TestCommitClampedAndMonotonic(t *testing.T) {
	f := NewFollower()
	f.HandleAppend(AppendReq{Term: 1, Entries: es(1, 1, 1, 1, 1)}) // 5 entries, none committed
	f.HandleAppend(AppendReq{Term: 1, PrevIndex: 0, Entries: es(1, 1), LeaderCommit: 9})
	if f.Commit != 2 {
		t.Fatalf("commit = %d; LeaderCommit 9 but the request only verified up to index 2", f.Commit)
	}
	f.HandleAppend(AppendReq{Term: 1, PrevIndex: 4, PrevTerm: 1, LeaderCommit: 4})
	if f.Commit != 4 {
		t.Fatalf("commit = %d; want 4", f.Commit)
	}
	f.HandleAppend(AppendReq{Term: 1, PrevIndex: 0, Entries: es(1), LeaderCommit: 1})
	if f.Commit != 4 {
		t.Fatalf("commit went back to %d on a delayed request", f.Commit)
	}
}

func TestEntriesAreCopied(t *testing.T) {
	f := NewFollower()
	buf := es(1, 1)
	f.HandleAppend(AppendReq{Term: 1, Entries: buf})
	buf[0] = Entry{9, "mutated"}
	if len(f.Log) == 0 || f.Log[0].Term != 1 {
		t.Fatalf("the follower's log aliases the request's slice")
	}
}

// ---- independent reference model: rebuilds the log instead of editing it ----

type refState struct {
	term, commit int
	log          []Entry
}

func (s refState) apply(r AppendReq) (refState, AppendResp) {
	if r.Term < s.term {
		return s, AppendResp{Term: s.term}
	}
	s.term = r.Term
	okPrev := r.PrevIndex == 0 || (r.PrevIndex <= len(s.log) && s.log[r.PrevIndex-1].Term == r.PrevTerm)
	if !okPrev {
		h := r.PrevIndex - 1
		if len(s.log) < h {
			h = len(s.log)
		}
		return s, AppendResp{Term: s.term, Match: h}
	}
	// length of the prefix of the request that already agrees with our log
	agree := 0
	for agree < len(r.Entries) && r.PrevIndex+agree < len(s.log) && s.log[r.PrevIndex+agree].Term == r.Entries[agree].Term {
		agree++
	}
	if agree < len(r.Entries) {
		nl := append([]Entry{}, s.log[:r.PrevIndex+agree]...)
		s.log = append(nl, r.Entries[agree:]...)
	}
	last := r.PrevIndex + len(r.Entries)
	if r.LeaderCommit > s.commit && min(r.LeaderCommit, last) > s.commit {
		s.commit = min(r.LeaderCommit, last)
	}
	return s, AppendResp{Term: s.term, Success: true, Match: last}
}

func TestRandomStreamsAgainstModel(t *testing.T) {
	for seed := int64(1); seed <= 200; seed++ {
		rng := rand.New(rand.NewSource(seed))
		// leader histories: each term's log extends a random prefix of the previous one
		var leaders [][]Entry
		cur := []Entry{}
		for term := 1; term <= 5; term++ {
			keep := 0
			if len(cur) > 0 {
				keep = rng.Intn(len(cur) + 1)
			}
			next := append([]Entry{}, cur[:keep]...)
			for k := 0; k < 1+rng.Intn(5); k++ {
				next = append(next, Entry{term, fmt.Sprintf("t%d-%d", term, len(next)+1)})
			}
			leaders = append(leaders, next)
			cur = next
		}
		f := NewFollower()
		ref := refState{}
		for step := 0; step < 150; step++ {
			lt := rng.Intn(len(leaders)) // which leader sends this request
			L := leaders[lt]
			prev := rng.Intn(len(L) + 1)
			if rng.Intn(3) == 0 && len(f.Log) > 0 {
				prev = min(len(f.Log), len(L)) // aim at the follower's end
			}
			pt := 0
			if prev > 0 {
				pt = L[prev-1].Term
			}
			if rng.Intn(15) == 0 {
				pt += 1 + rng.Intn(2) // garbage prev term
			}
			k := rng.Intn(len(L) - prev + 1)
			req := AppendReq{Term: lt + 1, Leader: lt, PrevIndex: prev, PrevTerm: pt,
				Entries: append([]Entry{}, L[prev:prev+k]...), LeaderCommit: rng.Intn(len(L) + 1)}
			var want AppendResp
			ref, want = ref.apply(req)
			got := f.HandleAppend(req)
			if got != want {
				t.Fatalf("seed %d step %d: request %+v\nresp %+v; model says %+v", seed, step, req, got, want)
			}
			if f.Term != ref.term || f.Commit != ref.commit || !reflect.DeepEqual(logOf(f), append([]Entry{}, ref.log...)) {
				t.Fatalf("seed %d step %d: after %+v\nfollower term %d commit %d log %v\nmodel    term %d commit %d log %v",
					seed, step, req, f.Term, f.Commit, f.Log, ref.term, ref.commit, ref.log)
			}
			if got.Success && !sameEntries(f.Log[:got.Match], L[:got.Match]) {
				t.Fatalf("seed %d step %d: log matching violated: accepted a request but log[:%d] = %v differs from the leader's %v", seed, step, got.Match, f.Log[:got.Match], L[:got.Match])
			}
		}
	}
}
