package main

import (
	"errors"
	"math/rand"
	"testing"
)

var base = State{Counter: 5, HighWater: 5, Digest: "d5", LastIssued: 1000}

func offer(counter uint64, digest string, issued, expires int64) Offer {
	return Offer{Version: "x", Counter: counter, IssuedAt: issued, ExpiresAt: expires, Digest: digest}
}

func TestRuleTable(t *testing.T) {
	cases := []struct {
		name string
		st   State
		o    Offer
		now  int64
		want error
	}{
		{"newer", base, offer(6, "d6", 1000, 5000), 1100, nil},
		{"counter zero in a fresh device", State{}, offer(0, "d", 0, 10), 5, ErrMalformed},
		{"much newer", base, offer(900, "dx", 1100, 5000), 1100, nil},
		{"same instant as last issued", base, offer(6, "d6", 1000, 5000), 1000, nil},
		{"replay", base, offer(5, "d5", 1000, 5000), 1100, ErrAlreadyInstalled},
		{"equivocation", base, offer(5, "evil", 1000, 5000), 1100, ErrEquivocation},
		{"old release", base, offer(4, "d4", 1000, 5000), 1100, ErrDowngrade},
		{"counter zero", base, offer(0, "d0", 1000, 5000), 1100, ErrMalformed},
		{"empty version", base, Offer{"", 6, 1000, 5000, "d6"}, 1100, ErrMalformed},
		{"empty digest", base, offer(6, "", 1000, 5000), 1100, ErrMalformed},
		{"expiry before issue", base, offer(6, "d6", 3000, 2000), 1100, ErrMalformed},
		{"expiry equals issue", base, offer(6, "d6", 3000, 3000), 1100, ErrMalformed},
		{"issued at now+skew", base, offer(6, "d6", 1400, 5000), 1100, nil},
		{"issued after now+skew", base, offer(6, "d6", 1401, 5000), 1100, ErrFuture},
		{"expires one tick ahead", base, offer(6, "d6", 1000, 1101), 1100, nil},
		{"expires exactly now", base, offer(6, "d6", 1000, 1100), 1100, ErrExpired},
		{"long expired", base, offer(6, "d6", 1000, 1050), 9000, ErrExpired},
		{"stale metadata", base, offer(6, "d6", 999, 5000), 1100, ErrStaleMetadata},
		{"stale beats downgrade", base, offer(2, "d2", 999, 5000), 1100, ErrStaleMetadata},
		{"expired beats stale", base, offer(6, "d6", 999, 1050), 1100, ErrExpired},
		{"future beats expired", base, offer(6, "d6", 2000, 1500), 1100, ErrMalformed},
		{"malformed beats future", base, Offer{"", 6, 9000, 9500, "d"}, 1100, ErrMalformed},
		{"replay but expired", base, offer(5, "d5", 1000, 1050), 1100, ErrExpired},
		{"fresh state accepts counter 1", State{}, offer(1, "d1", 0, 10), 5, nil},
		{"fresh state counter 0", State{}, offer(0, "d0", 0, 10), 5, ErrMalformed},
	}
	for _, c := range cases {
		err := Decide(c.st, c.o, c.now)
		if !errors.Is(err, c.want) && !(c.want == nil && err == nil) {
			t.Errorf("%s: got %v, want %v", c.name, err, c.want)
		}
	}
}

func TestApply(t *testing.T) {
	st := Apply(base, offer(6, "d6", 1200, 5000))
	if st != (State{6, 6, "d6", 1200}) {
		t.Errorf("Apply = %+v", st)
	}
	// An accepted offer with older IssuedAt cannot happen via Decide, but Apply must never move LastIssued back.
	st = Apply(State{Counter: 9, HighWater: 9, Digest: "d9", LastIssued: 5000}, offer(10, "d10", 100, 9000))
	if st.LastIssued != 5000 || st.HighWater != 10 {
		t.Errorf("Apply moved a monotonic field backwards: %+v", st)
	}
	rolled := State{Counter: 4, HighWater: 5, Digest: "d4", LastIssued: 1000}
	if st := Apply(rolled, offer(6, "d6", 1100, 5000)); st.HighWater != 6 || st.Counter != 6 {
		t.Errorf("Apply after rollback = %+v", st)
	}
}

func TestAttackerReplaysAfterLocalRollback(t *testing.T) {
	// Device ran 5, installed 6, 6 failed health checks and the device went back to 5.
	// HighWater stays at 6, so neither 6 nor 5 can be installed again; 7 can.
	st := State{Counter: 5, HighWater: 6, Digest: "d5", LastIssued: 1100}
	for _, o := range []Offer{offer(6, "d6", 1100, 9000), offer(6, "other", 1100, 9000), offer(4, "d4", 1100, 9000), offer(3, "d3", 1100, 9000)} {
		if err := Decide(st, o, 1200); err == nil {
			t.Errorf("accepted %+v after rollback", o)
		}
	}
	if err := Decide(st, offer(5, "d5", 1100, 9000), 1200); !errors.Is(err, ErrAlreadyInstalled) {
		t.Errorf("replaying the running version: %v", err)
	}
	if err := Decide(st, offer(7, "d7", 1300, 9000), 1400); err != nil {
		t.Errorf("fix release refused: %v", err)
	}
}

func TestFreezeAttack(t *testing.T) {
	// Attacker keeps serving the last signed metadata. It is valid until it expires,
	// and an older copy can never replace a newer one that has been seen.
	st := Apply(State{}, offer(3, "d3", 1000, 2000))
	if err := Decide(st, offer(3, "d3", 1000, 2000), 1500); !errors.Is(err, ErrAlreadyInstalled) {
		t.Errorf("frozen metadata inside its validity: %v", err)
	}
	if err := Decide(st, offer(4, "d4", 900, 9000), 1500); !errors.Is(err, ErrStaleMetadata) {
		t.Errorf("older-issued newer-counter offer: %v", err)
	}
	if err := Decide(st, offer(3, "d3", 1000, 2000), 2000); !errors.Is(err, ErrExpired) {
		t.Errorf("frozen metadata after expiry: %v", err)
	}
}

// reference: the rules as an ordered table of predicates, written separately.
func reference(st State, o Offer, now int64) error {
	type rule struct {
		hit bool
		err error
	}
	sameCounter := o.Counter == st.Counter
	rules := []rule{
		{o.Version == "" || o.Digest == "" || o.Counter == 0 || o.ExpiresAt <= o.IssuedAt, ErrMalformed},
		{o.IssuedAt-now > Skew, ErrFuture},
		{o.ExpiresAt <= now, ErrExpired},
		{st.LastIssued > o.IssuedAt, ErrStaleMetadata},
		{sameCounter && o.Digest == st.Digest, ErrAlreadyInstalled},
		{sameCounter, ErrEquivocation},
		{st.HighWater >= o.Counter || st.Counter > o.Counter, ErrDowngrade},
	}
	for _, r := range rules {
		if r.hit {
			return r.err
		}
	}
	return nil
}

func TestRandomAgainstReference(t *testing.T) {
	rng := rand.New(rand.NewSource(99))
	digests := []string{"a", "b", ""}
	for i := 0; i < 20000; i++ {
		hw := uint64(rng.Intn(8))
		c := hw
		if hw > 0 && rng.Intn(3) == 0 {
			c = uint64(rng.Intn(int(hw) + 1))
		}
		st := State{Counter: c, HighWater: hw, Digest: digests[rng.Intn(2)], LastIssued: int64(rng.Intn(20))}
		o := Offer{Version: []string{"", "v"}[rng.Intn(5)/4], Counter: uint64(rng.Intn(10)), IssuedAt: int64(rng.Intn(20)) - 3, Digest: digests[rng.Intn(3)]}
		o.ExpiresAt = o.IssuedAt + int64(rng.Intn(15)) - 2
		now := int64(rng.Intn(40)) - 5
		got, want := Decide(st, o, now), reference(st, o, now)
		if got != want && !(errors.Is(got, want) && want != nil) {
			t.Fatalf("Decide(%+v, %+v, now=%d) = %v; reference says %v", st, o, now, got, want)
		}
	}
}
