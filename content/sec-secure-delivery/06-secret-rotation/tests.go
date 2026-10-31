package main

import (
	"errors"
	"fmt"
	"math"
	"math/rand"
	"testing"
)

func sec(i int) string { return fmt.Sprintf("secret-number-%04d-padding", i) }

func TestGracePeriodTimeline(t *testing.T) {
	k := NewKeyRing(100)
	if v, err := k.Rotate(sec(1), 0); v != 1 || err != nil {
		t.Fatalf("first rotate = %d, %v", v, err)
	}
	if v, err := k.Rotate(sec(2), 1000); v != 2 || err != nil {
		t.Fatalf("second rotate = %d, %v", v, err)
	}
	type chk struct {
		secret string
		now    int64
		v      int
		ok     bool
	}
	for _, c := range []chk{
		{sec(1), 999, 1, true}, {sec(1), 1000, 1, true}, {sec(1), 1099, 1, true},
		{sec(1), 1100, 0, false}, {sec(1), 5000, 0, false},
		{sec(2), 999, 2, true}, {sec(2), 1100, 2, true}, {sec(2), 1 << 40, 2, true},
		{"wrong", 1000, 0, false}, {"", 1000, 0, false}, {sec(2) + "x", 1000, 0, false}, {sec(2)[:20], 1000, 0, false},
	} {
		if v, ok := k.Check(c.secret, c.now); v != c.v || ok != c.ok {
			t.Errorf("Check(%q, %d) = %d,%v; want %d,%v", c.secret, c.now, v, ok, c.v, c.ok)
		}
	}
	if k.Current() != 2 {
		t.Errorf("Current = %d", k.Current())
	}
}

func TestRevocationIsImmediate(t *testing.T) {
	k := NewKeyRing(1000)
	k.Rotate(sec(1), 0)
	k.Rotate(sec(2), 10)
	if _, ok := k.Check(sec(1), 20); !ok {
		t.Fatal("v1 should be in its grace period")
	}
	if err := k.Revoke(1); err != nil {
		t.Fatal(err)
	}
	if _, ok := k.Check(sec(1), 20); ok {
		t.Error("revoked v1 still accepted inside its grace period")
	}
	if err := k.Revoke(2); err != nil {
		t.Fatal(err)
	}
	if _, ok := k.Check(sec(2), 20); ok {
		t.Error("revoked current version accepted")
	}
	if k.Current() != 2 {
		t.Errorf("Current changes only on Rotate, got %d", k.Current())
	}
	// recovery: rotate after revoking the current version
	if v, err := k.Rotate(sec(3), 30); v != 3 || err != nil {
		t.Fatalf("rotate after revoke: %d %v", v, err)
	}
	if _, ok := k.Check(sec(2), 31); ok {
		t.Error("revoked v2 revived by the next rotation")
	}
	if v, ok := k.Check(sec(3), 31); v != 3 || !ok {
		t.Error("v3 should be valid")
	}
	for _, bad := range []int{0, -1, 4, 100} {
		if err := k.Revoke(bad); !errors.Is(err, ErrNoSuchVersion) {
			t.Errorf("Revoke(%d) = %v", bad, err)
		}
	}
}

func TestRotationRefusals(t *testing.T) {
	k := NewKeyRing(50)
	k.Rotate(sec(1), 100)
	k.Rotate(sec(2), 200)
	n := k.Current()
	for name, c := range map[string]struct {
		secret string
		now    int64
		want   error
	}{
		"weak":            {"short", 300, ErrWeak},
		"15 bytes":        {"123456789012345", 300, ErrWeak},
		"empty":           {"", 300, ErrWeak},
		"reuse current":   {sec(2), 300, ErrReuse},
		"reuse expired":   {sec(1), 300, ErrReuse},
		"clock backwards": {sec(3), 199, ErrClock},
	} {
		if _, err := k.Rotate(c.secret, c.now); !errors.Is(err, c.want) {
			t.Errorf("%s: got %v, want %v", name, err, c.want)
		}
	}
	if k.Current() != n {
		t.Error("failed rotations changed the ring")
	}
	// The failed backwards rotation must not have shortened or extended anything.
	if _, ok := k.Check(sec(1), 249); !ok {
		t.Error("v1 grace window disturbed")
	}
	if _, ok := k.Check(sec(1), 250); ok {
		t.Error("v1 outlived its grace window")
	}
	// 16 bytes is allowed; same instant is allowed.
	if v, err := k.Rotate("1234567890123456", 200); v != 3 || err != nil {
		t.Errorf("16-byte secret at same instant: %d %v", v, err)
	}
	// Reuse of a revoked secret is refused too.
	k.Revoke(3)
	if _, err := k.Rotate("1234567890123456", 400); !errors.Is(err, ErrReuse) {
		t.Errorf("revoked secret re-adopted: %v", err)
	}
}

func TestGraceNeverExtendsAnEarlierEnd(t *testing.T) {
	k := NewKeyRing(1000)
	k.Rotate(sec(1), 0)
	k.Rotate(sec(2), 100) // v1 ends 1100
	k.Rotate(sec(3), 200) // v2 ends 1200; v1 must still end at 1100
	if _, ok := k.Check(sec(1), 1099); !ok {
		t.Error("v1 should be valid until 1100")
	}
	if _, ok := k.Check(sec(1), 1100); ok {
		t.Error("a later rotation extended v1")
	}
	if _, ok := k.Check(sec(2), 1199); !ok {
		t.Error("v2 should be valid until 1200")
	}
	if _, ok := k.Check(sec(2), 1200); ok {
		t.Error("v2 outlived its grace")
	}
}

func TestZeroNegativeAndHugeGrace(t *testing.T) {
	for _, g := range []int64{0, -50} {
		k := NewKeyRing(g)
		k.Rotate(sec(1), 10)
		k.Rotate(sec(2), 20)
		if _, ok := k.Check(sec(1), 20); ok {
			t.Errorf("grace %d: old version should end at the rotation instant", g)
		}
	}
	k := NewKeyRing(math.MaxInt64)
	k.Rotate(sec(1), 10)
	k.Rotate(sec(2), 20)
	if _, ok := k.Check(sec(1), 1<<60); !ok {
		t.Error("huge grace overflowed to a past end time")
	}
}

type mver struct {
	secret  string
	endAt   int64
	revoked bool
}

// model is a deliberately different formulation: for every version it records the
// time of the *next* rotation and the grace, and computes validity from those.
type model struct {
	grace    int64
	vs       []mver
	rotTimes []int64
}

func (m *model) valid(secret string, now int64) int {
	best := 0
	for i, v := range m.vs {
		if v.secret != secret || v.revoked {
			continue
		}
		alive := true
		if i+1 < len(m.vs) {
			end := m.rotTimes[i+1]
			if m.grace > 0 {
				end += m.grace
			}
			alive = now < end
		}
		if alive {
			best = i + 1
		}
	}
	return best
}

func TestRandomSchedulesAgainstModel(t *testing.T) {
	for seed := int64(0); seed < 300; seed++ {
		rng := rand.New(rand.NewSource(seed))
		grace := int64(rng.Intn(60))
		k := NewKeyRing(grace)
		m := &model{grace: grace}
		now := int64(rng.Intn(10))
		issued := 0
		for step := 0; step < 80; step++ {
			now += int64(rng.Intn(40))
			switch r := rng.Intn(10); {
			case r < 2:
				issued++
				s := sec(issued)
				v, err := k.Rotate(s, now)
				if err != nil {
					t.Fatalf("seed %d: rotate failed: %v", seed, err)
				}
				m.vs = append(m.vs, mver{secret: s})
				m.rotTimes = append(m.rotTimes, now)
				if v != len(m.vs) {
					t.Fatalf("seed %d: version number %d, want %d", seed, v, len(m.vs))
				}
			case r == 2 && issued > 0:
				v := 1 + rng.Intn(issued)
				if err := k.Revoke(v); err != nil {
					t.Fatalf("seed %d: revoke: %v", seed, err)
				}
				m.vs[v-1].revoked = true
			case r == 3 && issued > 0:
				// attempted reuse or clock regression must leave things as they are
				if _, err := k.Rotate(sec(1+rng.Intn(issued)), now); !errors.Is(err, ErrReuse) {
					t.Fatalf("seed %d: reuse gave %v", seed, err)
				}
				if _, err := k.Rotate(sec(issued+500), m.rotTimes[len(m.rotTimes)-1]-1); !errors.Is(err, ErrClock) {
					t.Fatalf("seed %d: backwards clock gave %v", seed, err)
				}
			default:
				cands := []string{"junk", sec(issued + 1)}
				for i := 1; i <= issued; i++ {
					cands = append(cands, sec(i))
				}
				for _, c := range cands {
					for _, at := range []int64{now, now + int64(rng.Intn(120))} {
						want := m.valid(c, at)
						v, ok := k.Check(c, at)
						if v != want || ok != (want > 0) {
							t.Fatalf("seed %d step %d: Check(%q, %d) = %d,%v; model says %d", seed, step, c, at, v, ok, want)
						}
					}
				}
			}
			if k.Current() != len(m.vs) {
				t.Fatalf("seed %d: Current = %d, want %d", seed, k.Current(), len(m.vs))
			}
		}
	}
}
