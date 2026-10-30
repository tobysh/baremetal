package main

import (
	"fmt"
	"math/rand"
	"testing"
)

type netMsg struct {
	req  *Request
	resp *Response
}

type simClient struct {
	id       string
	c        *Client
	amounts  []int
	expected int // sequence number the client must be asking for
}

func runSchedule(t *testing.T, seed int64, nClients int) {
	t.Helper()
	rng := rand.New(rand.NewSource(seed))
	srv := NewServer()
	var cls []*simClient
	byID := map[string]*simClient{}
	for i := 0; i < nClients; i++ {
		id := fmt.Sprintf("c%d", i)
		am := make([]int, 2+rng.Intn(5))
		for j := range am {
			am[j] = rng.Intn(21) - 8
		}
		sc := &simClient{id: id, c: NewClient(id, append([]int(nil), am...)), amounts: am, expected: 1}
		cls = append(cls, sc)
		byID[id] = sc
	}
	applied := map[string]int{}
	first := map[string]int{}
	var pool []netMsg
	want := func() int {
		sum := 0
		for _, sc := range cls {
			for _, a := range sc.amounts[:applied[sc.id]] {
				sum += a
			}
		}
		return sum
	}
	for step := 0; step < 30000; step++ {
		done := true
		for _, sc := range cls {
			if sc.expected <= len(sc.amounts) {
				done = false
			}
		}
		if done {
			break
		}
		chaos := step < 700
		if len(pool) == 0 || rng.Intn(3) == 0 {
			sc := cls[rng.Intn(len(cls))]
			req, ok := sc.c.Pending()
			if sc.expected > len(sc.amounts) {
				if ok || !sc.c.Done() {
					t.Fatalf("seed %d: client %s has every reply but Pending()=%v ok=%v Done()=%v", seed, sc.id, req, ok, sc.c.Done())
				}
				continue
			}
			if !ok || sc.c.Done() {
				t.Fatalf("seed %d: client %s still has request %d to send but Pending() ok=%v Done()=%v", seed, sc.id, sc.expected, ok, sc.c.Done())
			}
			if req.Client != sc.id || req.Seq != sc.expected || req.Amount != sc.amounts[sc.expected-1] {
				t.Fatalf("seed %d: client %s should be sending {%s %d %d}, Pending() gave %+v", seed, sc.id, sc.id, sc.expected, sc.amounts[sc.expected-1], req)
			}
			if !chaos || rng.Intn(10) >= 3 {
				r := req
				pool = append(pool, netMsg{req: &r})
			}
			if chaos && rng.Intn(4) == 0 {
				r := req
				pool = append(pool, netMsg{req: &r})
			}
			continue
		}
		i := rng.Intn(len(pool))
		m := pool[i]
		if !(chaos && rng.Intn(7) == 0) { // in chaos, sometimes the message stays and is delivered again later
			pool[i] = pool[len(pool)-1]
			pool = pool[:len(pool)-1]
		}
		if chaos && rng.Intn(5) == 0 {
			continue // dropped
		}
		if m.req != nil {
			r := *m.req
			latest := r.Seq == applied[r.Client]
			resp := srv.Handle(r)
			if r.Seq > applied[r.Client] {
				applied[r.Client] = r.Seq
			}
			if got := srv.Balance(); got != want() {
				t.Fatalf("seed %d: after delivering %+v the balance is %d; applying every request exactly once gives %d", seed, r, got, want())
			}
			if resp.Client != r.Client || resp.Seq != r.Seq {
				t.Fatalf("seed %d: reply %+v does not echo request %+v", seed, resp, r)
			}
			key := fmt.Sprintf("%s/%d", r.Client, r.Seq)
			if prev, ok := first[key]; ok {
				if latest && resp.Balance != prev {
					t.Fatalf("seed %d: duplicate of %s answered balance %d, the first answer was %d", seed, key, resp.Balance, prev)
				}
			} else {
				first[key] = resp.Balance
				if resp.Balance != srv.Balance() {
					t.Fatalf("seed %d: reply to %s says balance %d, server has %d", seed, key, resp.Balance, srv.Balance())
				}
			}
			if !chaos || rng.Intn(4) != 0 {
				pool = append(pool, netMsg{resp: &resp})
			}
		} else {
			resp := *m.resp
			sc := byID[resp.Client]
			sc.c.OnResponse(resp)
			if resp.Seq == sc.expected && sc.expected <= len(sc.amounts) {
				sc.expected++
			}
		}
	}
	for _, sc := range cls {
		if !sc.c.Done() || sc.expected <= len(sc.amounts) {
			t.Fatalf("seed %d: client %s never finished; does Pending() keep returning the unacknowledged request?", seed, sc.id)
		}
	}
	total := 0
	for _, sc := range cls {
		for _, a := range sc.amounts {
			total += a
		}
	}
	if srv.Balance() != total {
		t.Fatalf("seed %d: final balance %d; want %d", seed, srv.Balance(), total)
	}
}

func TestDuplicateRequestAppliedOnce(t *testing.T) {
	s := NewServer()
	r1 := s.Handle(Request{"a", 1, 10})
	r2 := s.Handle(Request{"a", 1, 10})
	if s.Balance() != 10 {
		t.Fatalf("balance after a duplicated request = %d; want 10", s.Balance())
	}
	if r1 != r2 || r1.Balance != 10 {
		t.Fatalf("duplicate got %+v; want exactly the first reply %+v with balance 10", r2, r1)
	}
	s.Handle(Request{"b", 1, 7})
	r3 := s.Handle(Request{"a", 1, 10})
	if r3 != r1 || s.Balance() != 17 {
		t.Fatalf("a duplicate after another client's request: reply %+v balance %d; want %+v and 17", r3, s.Balance(), r1)
	}
}

func TestOldRequestIgnored(t *testing.T) {
	s := NewServer()
	s.Handle(Request{"a", 1, 5})
	s.Handle(Request{"a", 2, 6})
	resp := s.Handle(Request{"a", 1, 5})
	if s.Balance() != 11 {
		t.Fatalf("a late copy of request 1 changed the balance to %d; want 11", s.Balance())
	}
	if resp.Client != "a" || resp.Seq != 1 {
		t.Fatalf("reply to the late copy = %+v; want Client a, Seq 1", resp)
	}
}

func TestZeroAmountStillCounts(t *testing.T) {
	s := NewServer()
	s.Handle(Request{"a", 1, 0})
	r := s.Handle(Request{"a", 2, 4})
	if r.Balance != 4 || s.Balance() != 4 {
		t.Fatalf("zero then four: balance %d; want 4 (a zero amount is still a request)", s.Balance())
	}
}

func TestClientIgnoresStaleReplies(t *testing.T) {
	c := NewClient("a", []int{1, 2, 3})
	c.OnResponse(Response{"a", 1, 1})
	req, _ := c.Pending()
	if req.Seq != 2 || req.Amount != 2 {
		t.Fatalf("after reply 1 the client should send request 2; Pending() = %+v", req)
	}
	c.OnResponse(Response{"a", 1, 1}) // late copy
	c.OnResponse(Response{"b", 2, 9}) // someone else's reply
	c.OnResponse(Response{"a", 3, 9}) // from the future
	req, _ = c.Pending()
	if req.Seq != 2 {
		t.Fatalf("a stale, foreign or future reply moved the client to request %d; it must still be on 2", req.Seq)
	}
	c.OnResponse(Response{"a", 2, 3})
	c.OnResponse(Response{"a", 3, 6})
	if !c.Done() {
		t.Fatalf("Done() is false after all three replies")
	}
	if _, ok := c.Pending(); ok {
		t.Fatalf("Pending() still offers a request after Done()")
	}
}

func TestPendingIsStable(t *testing.T) {
	c := NewClient("a", []int{4, 5})
	r1, _ := c.Pending()
	r2, _ := c.Pending()
	if r1 != r2 {
		t.Fatalf("Pending() changed from %+v to %+v without a reply; a retry must repeat the same request", r1, r2)
	}
}

func TestEmptyClientIsDone(t *testing.T) {
	c := NewClient("a", nil)
	if !c.Done() {
		t.Fatalf("a client with nothing to send should be Done")
	}
}

func TestSchedules(t *testing.T) {
	for seed := int64(1); seed <= 150; seed++ {
		runSchedule(t, seed, 1+int(seed%4))
		if t.Failed() {
			return
		}
	}
}
