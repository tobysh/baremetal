package main

import (
	"fmt"
	"math/rand"
	"testing"
)

type fault int

const (
	reliable fault = iota
	down
	flaky
)

// simPeer wraps a Replica with the failure mode chosen for the current operation.
type simPeer struct {
	rep     *Replica
	mode    fault
	rng     *rand.Rand
	getOK   int
	putOK   int
	putCall int
}

func (p *simPeer) Get() (Versioned, error) {
	switch p.mode {
	case down:
		return Versioned{}, fmt.Errorf("down")
	case flaky:
		if p.rng.Intn(100) < 25 {
			return Versioned{}, fmt.Errorf("lost")
		}
	}
	p.getOK++
	return p.rep.Get()
}

func (p *simPeer) Put(v Versioned) error {
	p.putCall++
	switch p.mode {
	case down:
		return fmt.Errorf("down")
	case flaky:
		switch r := p.rng.Intn(100); {
		case r < 15:
			return fmt.Errorf("lost before")
		case r < 30:
			_ = p.rep.Put(v) // applied, acknowledgement lost
			return fmt.Errorf("lost after")
		}
	}
	p.putOK++
	return p.rep.Put(v)
}

func newCluster(n int, rng *rand.Rand) ([]*Replica, []*simPeer, []Peer) {
	reps := make([]*Replica, n)
	sps := make([]*simPeer, n)
	peers := make([]Peer, n)
	for i := range reps {
		reps[i] = &Replica{}
		sps[i] = &simPeer{rep: reps[i], rng: rng}
		peers[i] = sps[i]
	}
	return reps, sps, peers
}

func TestHealthyRoundTrip(t *testing.T) {
	_, _, peers := newCluster(3, rand.New(rand.NewSource(1)))
	reg := NewRegister(peers)
	if v, err := reg.Read(); err != nil || v != "" {
		t.Fatalf("fresh register read = %q, %v; want empty and nil", v, err)
	}
	for _, s := range []string{"a", "b", "c"} {
		if err := reg.Write(s); err != nil {
			t.Fatalf("Write(%q) = %v on healthy replicas", s, err)
		}
		if v, err := reg.Read(); err != nil || v != s {
			t.Fatalf("after Write(%q) Read = %q, %v", s, v, err)
		}
	}
}

func TestMajorityDownStillWorks(t *testing.T) {
	_, sps, peers := newCluster(5, rand.New(rand.NewSource(1)))
	reg := NewRegister(peers)
	sps[0].mode, sps[3].mode = down, down
	if err := reg.Write("x"); err != nil {
		t.Fatalf("Write with 3 of 5 replicas up failed: %v", err)
	}
	sps[0].mode, sps[3].mode = reliable, reliable
	sps[1].mode, sps[2].mode = down, down // the two that never saw "x" form the only... plus one that did
	if v, err := reg.Read(); err != nil || v != "x" {
		t.Fatalf("Read via a different majority = %q, %v; want x", v, err)
	}
}

func TestNoQuorumFails(t *testing.T) {
	_, sps, peers := newCluster(3, rand.New(rand.NewSource(1)))
	reg := NewRegister(peers)
	sps[0].mode, sps[1].mode = down, down
	if err := reg.Write("x"); err == nil {
		t.Fatalf("Write succeeded with only 1 of 3 replicas reachable")
	}
	if _, err := reg.Read(); err == nil {
		t.Fatalf("Read succeeded with only 1 of 3 replicas reachable")
	}
}

// A write that lands on one replica but is not acknowledged must not poison
// later writes: the next write needs a fresh, higher version.
func TestFailedWriteDoesNotReuseVersion(t *testing.T) {
	reps, sps, peers := newCluster(3, rand.New(rand.NewSource(1)))
	reg := NewRegister(peers)
	if err := reg.Write("one"); err != nil {
		t.Fatal(err)
	}
	// "two" reaches only replica 0, and is reported as failed.
	failPut := &putScript{inner: sps[0], applyAndFail: true}
	deadPut1, deadPut2 := &putScript{inner: sps[1], blockPut: true}, &putScript{inner: sps[2], blockPut: true}
	reg2 := &Register{peers: []Peer{failPut, deadPut1, deadPut2}}
	reg2.last = reg.last
	_ = reg2
	// Drive the real register through the same situation.
	reg.peers = []Peer{failPut, deadPut1, deadPut2}
	if err := reg.Write("two"); err == nil {
		t.Fatalf("Write reported success though 2 of 3 Puts failed")
	}
	reg.peers = peers
	if err := reg.Write("three"); err != nil {
		t.Fatal(err)
	}
	seen := map[int]string{}
	for _, r := range reps {
		if r.cur.Ver == 0 {
			continue
		}
		if prev, ok := seen[r.cur.Ver]; ok && prev != r.cur.Val {
			t.Fatalf("version %d holds both %q and %q: a failed write's version was reused", r.cur.Ver, prev, r.cur.Val)
		}
		seen[r.cur.Ver] = r.cur.Val
	}
	for i := 0; i < 3; i++ {
		if v, err := reg.Read(); err != nil || v != "three" {
			t.Fatalf("Read = %q, %v; want three", v, err)
		}
	}
}

type putScript struct {
	inner        *simPeer
	applyAndFail bool
	blockPut     bool
}

func (p *putScript) Get() (Versioned, error) { return p.inner.Get() }
func (p *putScript) Put(v Versioned) error {
	if p.blockPut {
		return fmt.Errorf("blocked")
	}
	if p.applyAndFail {
		_ = p.inner.rep.Put(v)
		return fmt.Errorf("ack lost")
	}
	return p.inner.Put(v)
}

// A read that returned a partially written value must make it stick.
func TestReadRepairsSoLaterReadsDoNotGoBack(t *testing.T) {
	reps, sps, peers := newCluster(3, rand.New(rand.NewSource(1)))
	reg := NewRegister(peers)
	if err := reg.Write("old"); err != nil {
		t.Fatal(err)
	}
	reps[0].Put(Versioned{reg.last + 1, "new"}) // a failed write left this on replica 0 only
	v, err := reg.Read()
	if err != nil {
		t.Fatal(err)
	}
	if v != "new" {
		t.Skipf("first read returned %q", v)
	}
	sps[0].mode = down // the only replica that held "new" is gone
	for i := 0; i < 3; i++ {
		got, err := reg.Read()
		if err != nil || got != "new" {
			t.Fatalf("read %d after the first read returned new: %q, %v; a value that was read must stay readable (write it back to a quorum)", i, got, err)
		}
	}
}

func TestSchedules(t *testing.T) {
	for seed := int64(1); seed <= 120; seed++ {
		n := 3
		if seed%2 == 0 {
			n = 5
		}
		runRegisterSchedule(t, seed, n)
		if t.Failed() {
			return
		}
	}
}

func runRegisterSchedule(t *testing.T, seed int64, n int) {
	rng := rand.New(rand.NewSource(seed))
	reps, sps, peers := newCluster(n, rng)
	reg := NewRegister(peers)
	q := n/2 + 1
	order := map[string]int{"": 0} // value -> attempt number
	acked := ""                    // last acknowledged value
	allowed := map[string]bool{"": true}
	lastRead := 0
	attempts := 0
	for op := 0; op < 120; op++ {
		// choose a failure plan for this operation
		kind := rng.Intn(3)
		up := q + rng.Intn(n-q+1)
		if kind == 1 {
			up = rng.Intn(q) // fewer than a quorum
		}
		perm := rng.Perm(n)
		for _, sp := range sps {
			sp.getOK, sp.putOK, sp.putCall = 0, 0, 0
		}
		for k, idx := range perm {
			switch {
			case kind == 2:
				sps[idx].mode = flaky
			case k < up:
				sps[idx].mode = reliable
			default:
				sps[idx].mode = down
			}
		}
		total := func(f func(*simPeer) int) int {
			s := 0
			for _, sp := range sps {
				s += f(sp)
			}
			return s
		}
		if rng.Intn(2) == 0 {
			attempts++
			val := fmt.Sprintf("v%d", attempts)
			order[val] = attempts
			err := reg.Write(val)
			if err == nil {
				if total(func(p *simPeer) int { return p.putOK }) < q {
					t.Fatalf("seed %d op %d: Write returned nil with only %d acknowledged Puts (need %d)", seed, op, total(func(p *simPeer) int { return p.putOK }), q)
				}
				acked = val
				allowed = map[string]bool{val: true}
			} else {
				allowed[val] = true
				if kind == 0 {
					t.Fatalf("seed %d op %d: Write failed (%v) although %d of %d replicas were reliably reachable", seed, op, err, up, n)
				}
			}
			if kind == 1 && err == nil {
				t.Fatalf("seed %d op %d: Write succeeded with only %d of %d replicas reachable", seed, op, up, n)
			}
		} else {
			v, err := reg.Read()
			switch {
			case err != nil && kind == 0:
				t.Fatalf("seed %d op %d: Read failed (%v) although %d of %d replicas were reliably reachable", seed, op, err, up, n)
			case err == nil && kind == 1:
				t.Fatalf("seed %d op %d: Read succeeded with only %d of %d replicas reachable", seed, op, up, n)
			case err == nil:
				if !allowed[v] {
					t.Fatalf("seed %d op %d: Read returned %q; the last acknowledged value was %q and only later attempts could appear", seed, op, v, acked)
				}
				if order[v] < lastRead {
					t.Fatalf("seed %d op %d: Read returned %q (attempt %d) after an earlier read returned attempt %d: the register went back in time", seed, op, v, order[v], lastRead)
				}
				lastRead = order[v]
			}
		}
		seen := map[int]string{}
		for _, r := range reps {
			if r.cur.Ver == 0 {
				continue
			}
			if prev, ok := seen[r.cur.Ver]; ok && prev != r.cur.Val {
				t.Fatalf("seed %d op %d: version %d holds both %q and %q", seed, op, r.cur.Ver, prev, r.cur.Val)
			}
			seen[r.cur.Ver] = r.cur.Val
		}
	}
}
