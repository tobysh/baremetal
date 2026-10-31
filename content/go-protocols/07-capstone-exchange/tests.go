package main

import (
	"bytes"
	"errors"
	"math/rand"
	"testing"
)

// ---- deterministic fault-injecting pipe ----

type pipe struct {
	r                  *rand.Rand
	drop, corrupt, dup int // percent
	maxChunk           int
	q                  [][]byte
}

func (p *pipe) write(b []byte) {
	if len(b) == 0 || p.r.Intn(100) < p.drop {
		return
	}
	copies := 1
	if p.r.Intn(100) < p.dup {
		copies = 2
	}
	for ; copies > 0; copies-- {
		c := append([]byte(nil), b...)
		if p.r.Intn(100) < p.corrupt {
			c[p.r.Intn(len(c))] ^= byte(1 + p.r.Intn(255))
		}
		for len(c) > 0 {
			n := 1 + p.r.Intn(p.maxChunk)
			if n > len(c) {
				n = len(c)
			}
			p.q = append(p.q, c[:n])
			c = c[n:]
		}
	}
}

func (p *pipe) take() [][]byte { q := p.q; p.q = nil; return q }

func reverse(b []byte) []byte {
	o := make([]byte, len(b))
	for i := range b {
		o[len(b)-1-i] = b[i]
	}
	return o
}

// observer parses what the client emits, to check the window independently.
type observer struct {
	sc   *Scanner
	seen map[uint32]bool
}

func (o *observer) see(c *Client, b []byte) int {
	frames, _ := o.sc.Feed(b)
	for _, f := range frames {
		if f.Type == TypeReq && len(f.Payload) >= 4 {
			o.seen[uint32(f.Payload[0])<<24|uint32(f.Payload[1])<<16|uint32(f.Payload[2])<<8|uint32(f.Payload[3])] = true
		}
	}
	open := 0
	for seq := range o.seen {
		if c.Results()[seq-1] == nil {
			open++
		}
	}
	return open
}

type scenario struct {
	seed               int64
	drop, corrupt, dup int
	maxChunk           int
	client, server     Hello
	reqs               [][]byte
}

func makeReqs(r *rand.Rand, n int) [][]byte {
	out := make([][]byte, n)
	for i := range out {
		sizes := []int{0, 1, 9, 100, 1020}
		out[i] = make([]byte, sizes[r.Intn(len(sizes))])
		r.Read(out[i])
	}
	return out
}

func simulate(t *testing.T, sc scenario) (*Client, *Server, int) {
	t.Helper()
	defer func() {
		if e := recover(); e != nil {
			t.Fatalf("seed %d: panic: %v", sc.seed, e)
		}
	}()
	r := rand.New(rand.NewSource(sc.seed))
	c2s := &pipe{r: r, drop: sc.drop, corrupt: sc.corrupt, dup: sc.dup, maxChunk: sc.maxChunk}
	s2c := &pipe{r: r, drop: sc.drop, corrupt: sc.corrupt, dup: sc.dup, maxChunk: sc.maxChunk}
	cli := NewClient(sc.client, sc.reqs)
	srv := NewServer(sc.server)
	obs := &observer{sc: NewScanner(), seen: map[uint32]bool{}}
	now := 0
	send := func(b []byte) {
		if open := obs.see(cli, b); open > Window {
			t.Fatalf("seed %d tick %d: %d requests unanswered; window is %d", sc.seed, now, open, Window)
		}
		c2s.write(b)
	}
	for ; now < 3000 && !cli.Done(); now++ {
		send(cli.Tick(now))
		for _, ch := range s2c.take() {
			send(cli.OnData(now, ch))
		}
		for _, ch := range c2s.take() {
			s2c.write(srv.OnData(ch))
		}
	}
	return cli, srv, now
}

var fullHello = Hello{MinVer: 1, MaxVer: 3, Features: FeatAck | FeatCompress | FeatBatch}

func TestPerfect_link(t *testing.T) {
	sc := scenario{seed: 1, maxChunk: 100000, client: Hello{1, 2, FeatAck | 1<<9, 0}, server: fullHello,
		reqs: [][]byte{[]byte("abc"), {}, []byte("hello world")}}
	cli, srv, now := simulate(t, sc)
	if !cli.Done() || cli.Err() != nil {
		t.Fatalf("done=%v err=%v", cli.Done(), cli.Err())
	}
	for i, q := range sc.reqs {
		if !bytes.Equal(cli.Results()[i], reverse(q)) || cli.Results()[i] == nil {
			t.Errorf("result %d = %q; want %q", i, cli.Results()[i], reverse(q))
		}
	}
	if cli.Version() != 2 || cli.Features() != FeatAck {
		t.Errorf("negotiated version %d features %b; want 2 and ack only", cli.Version(), cli.Features())
	}
	if now > 20 || !srv.Closed() {
		t.Errorf("finished at tick %d, server closed=%v; a clean link should need only a few ticks and end with BYE", now, srv.Closed())
	}
}

func TestFaulty_links_many_seeds(t *testing.T) {
	for seed := int64(1); seed <= 60; seed++ {
		r := rand.New(rand.NewSource(seed))
		sc := scenario{seed: seed, drop: 10, corrupt: 10, dup: 10, maxChunk: []int{1, 3, 11, 400}[seed%4],
			client: Hello{1, 3, FeatAck | FeatBatch, 0}, server: Hello{2, 3, FeatAck | FeatCompress | FeatBatch, 0},
			reqs: makeReqs(r, 1+int(seed%12))}
		cli, srv, now := simulate(t, sc)
		if !cli.Done() || cli.Err() != nil {
			t.Fatalf("seed %d: done=%v err=%v at tick %d", seed, cli.Done(), cli.Err(), now)
		}
		for i, q := range sc.reqs {
			if cli.Results()[i] == nil || !bytes.Equal(cli.Results()[i], reverse(q)) {
				t.Fatalf("seed %d: result %d wrong (len %d, want %d)", seed, i, len(cli.Results()[i]), len(q))
			}
		}
		if cli.Version() != 3 || cli.Features() != FeatAck|FeatBatch {
			t.Fatalf("seed %d: version %d features %b", seed, cli.Version(), cli.Features())
		}
		_ = srv
	}
}

func TestHeavy_duplication_and_single_byte_chunks(t *testing.T) {
	r := rand.New(rand.NewSource(7))
	sc := scenario{seed: 7, dup: 60, maxChunk: 1, client: fullHello, server: fullHello, reqs: makeReqs(r, 10)}
	cli, _, _ := simulate(t, sc)
	if cli.Err() != nil || !cli.Done() {
		t.Fatalf("done=%v err=%v", cli.Done(), cli.Err())
	}
	for i, q := range sc.reqs {
		if !bytes.Equal(cli.Results()[i], reverse(q)) {
			t.Fatalf("result %d wrong", i)
		}
	}
}

func TestDead_link_gives_up(t *testing.T) {
	sc := scenario{seed: 3, drop: 100, maxChunk: 10, client: fullHello, server: fullHello, reqs: [][]byte{[]byte("x")}}
	cli, _, now := simulate(t, sc)
	if !cli.Done() || !errors.Is(cli.Err(), ErrGiveUp) {
		t.Fatalf("done=%v err=%v; want ErrGiveUp", cli.Done(), cli.Err())
	}
	if now > (MaxRetries+2)*RetryTicks {
		t.Errorf("took %d ticks to give up; budget is about %d", now, (MaxRetries+2)*RetryTicks)
	}
}

func TestDead_return_path_after_handshake(t *testing.T) {
	// Server replies are all dropped after WELCOME is delivered: simulate directly.
	cli := NewClient(fullHello, [][]byte{[]byte("a"), []byte("b")})
	srv := NewServer(fullHello)
	out := cli.Tick(0)
	cli.OnData(0, srv.OnData(out)) // WELCOME arrives, requests go out
	sent := 0
	for now := 1; now < 1000 && !cli.Done(); now++ {
		if b := cli.Tick(now); len(b) > 0 {
			sent++ // responses never come back
		}
	}
	if !errors.Is(cli.Err(), ErrGiveUp) || sent > 2*(MaxRetries+1) {
		t.Errorf("err=%v after %d transmissions; want ErrGiveUp with bounded retries", cli.Err(), sent)
	}
}

func TestIncompatible_server_is_refused_without_requests(t *testing.T) {
	r := rand.New(rand.NewSource(1))
	sc := scenario{seed: 5, maxChunk: 7, client: Hello{3, 3, 0, 0}, server: Hello{1, 2, 0, 0}, reqs: makeReqs(r, 3)}
	cli, srv, _ := simulate(t, sc)
	if !cli.Done() || !errors.Is(cli.Err(), ErrRejected) {
		t.Fatalf("done=%v err=%v; want ErrRejected", cli.Done(), cli.Err())
	}
	for _, res := range cli.Results() {
		if res != nil {
			t.Errorf("a result exists for a refused session")
		}
	}
	// the server saw no REQ: ask it directly
	frames, _ := NewScanner().Feed(srv.OnData(mustEncode(TypeReq, []byte{0, 0, 0, 1, 'x'})))
	if len(frames) != 1 || frames[0].Type != TypeErr {
		t.Errorf("server accepted a request without a session")
	}
}

func TestRequired_feature_missing_is_refused(t *testing.T) {
	sc := scenario{seed: 6, maxChunk: 9, client: Hello{1, 3, FeatAck, FeatCompress}, server: Hello{1, 3, FeatAck, 0}, reqs: [][]byte{[]byte("q")}}
	cli, _, _ := simulate(t, sc)
	if !errors.Is(cli.Err(), ErrRejected) {
		t.Errorf("err=%v; want ErrRejected", cli.Err())
	}
}

func TestServer_survives_hostile_input_then_serves(t *testing.T) {
	for seed := int64(0); seed < 30; seed++ {
		r := rand.New(rand.NewSource(seed))
		srv := NewServer(fullHello)
		func() {
			defer func() {
				if e := recover(); e != nil {
					t.Fatalf("seed %d: server panicked: %v", seed, e)
				}
			}()
			junk := make([]byte, 3000)
			for i := range junk {
				if r.Intn(10) == 0 {
					junk[i] = 0xBE
				} else {
					junk[i] = byte(r.Intn(256))
				}
			}
			for len(junk) > 0 {
				n := 1 + r.Intn(300)
				if n > len(junk) {
					n = len(junk)
				}
				srv.OnData(junk[:n])
				junk = junk[n:]
			}
			// frames of the wrong shape
			srv.OnData(mustEncode(TypeReq, []byte{1}))   // short request, before and after
			srv.OnData(mustEncode(TypeHello, []byte{1})) // short hello
			srv.OnData(mustEncode(99, []byte("??")))     // unknown type
			srv.OnData([]byte{0xBE, 3, 0xFF, 0xFF})      // oversize header
		}()
		var in []byte
		in = append(in, mustEncode(TypeHello, EncodeHello(Hello{1, 3, FeatAck, 0}))...)
		in = append(in, mustEncode(TypeReq, []byte{0, 0, 0, 5, 'a', 'b', 'c'})...)
		in = append(in, make([]byte, 1100)...) // lets any pending false header resolve
		frames, _ := NewScanner().Feed(srv.OnData(in))
		var resp *Frame
		for i := range frames {
			if frames[i].Type == TypeResp {
				resp = &frames[i]
			}
		}
		if resp == nil || !bytes.Equal(resp.Payload, []byte{0, 0, 0, 5, 'c', 'b', 'a'}) {
			t.Fatalf("seed %d: server did not recover and answer (got %d frames)", seed, len(frames))
		}
	}
}

func TestClient_ignores_forged_and_duplicate_responses(t *testing.T) {
	srv := NewServer(fullHello)
	cli := NewClient(fullHello, [][]byte{[]byte("ab"), []byte("cd"), []byte("ef"), []byte("gh"), []byte("ij")})
	reply := srv.OnData(cli.Tick(0))
	out := cli.OnData(0, reply)
	// responses for request 4 and 5 were never sent; a forged RESP for them must not count
	forged := append(mustEncode(TypeResp, []byte{0, 0, 0, 4, 'X'}), mustEncode(TypeResp, []byte{0, 0, 0, 5, 'X'})...)
	forged = append(forged, mustEncode(TypeResp, []byte{0, 0, 0, 0, 'X'})...)
	forged = append(forged, mustEncode(TypeResp, []byte{0, 0, 0, 99, 'X'})...)
	forged = append(forged, mustEncode(TypeResp, []byte{1, 2})...)
	forged = append(forged, mustEncode(TypeErr, []byte("late"))...)
	forged = append(forged, mustEncode(TypeWelcome, EncodeAgreement(Agreement{3, 0}))...)
	cli.OnData(1, forged)
	if cli.Results()[3] != nil || cli.Results()[4] != nil || cli.Done() || cli.Err() != nil {
		t.Fatalf("forged responses were accepted: %q done=%v err=%v", cli.Results(), cli.Done(), cli.Err())
	}
	// A real answer, delivered twice, counts once and the result stays the first one.
	real := srv.OnData(out)
	cli.OnData(2, real)
	cli.OnData(2, mustEncode(TypeResp, []byte{0, 0, 0, 1, 'Z', 'Z'}))
	if string(cli.Results()[0]) != "ba" {
		t.Errorf("result 0 = %q; want ba (a duplicate must not overwrite)", cli.Results()[0])
	}
}

func TestClient_rejects_unacceptable_welcome(t *testing.T) {
	cli := NewClient(Hello{1, 2, FeatAck, 0}, [][]byte{[]byte("a")})
	cli.Tick(0)
	cli.OnData(0, mustEncode(TypeWelcome, EncodeAgreement(Agreement{5, FeatAck})))
	if !cli.Done() || cli.Err() == nil {
		t.Errorf("a WELCOME for version 5 was accepted by a client that speaks 1-2")
	}
	cli = NewClient(Hello{1, 2, FeatAck, 0}, [][]byte{[]byte("a")})
	cli.Tick(0)
	cli.OnData(0, mustEncode(TypeWelcome, EncodeAgreement(Agreement{2, FeatAck | FeatBatch})))
	if !cli.Done() || cli.Err() == nil {
		t.Errorf("a WELCOME enabling a feature the client never offered was accepted")
	}
}

func TestNo_requests_just_hello_and_bye(t *testing.T) {
	cli, srv, _ := simulate(t, scenario{seed: 11, drop: 20, corrupt: 10, maxChunk: 5, client: fullHello, server: fullHello})
	if !cli.Done() || cli.Err() != nil || len(cli.Results()) != 0 {
		t.Fatalf("done=%v err=%v", cli.Done(), cli.Err())
	}
	_ = srv
}
