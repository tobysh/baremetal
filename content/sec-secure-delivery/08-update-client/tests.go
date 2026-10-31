package main

import (
	"bytes"
	"crypto/ed25519"
	"errors"
	"fmt"
	"math/rand"
	"reflect"
	"testing"
)

func key(name string) (ed25519.PrivateKey, Config) {
	seed := make([]byte, ed25519.SeedSize)
	copy(seed, name)
	p := ed25519.NewKeyFromSeed(seed)
	return p, Config{Pub: p.Public().(ed25519.PublicKey)}
}

var priv, cfg = key("capstone-tests-key")

func body(v int) []byte {
	return []byte(fmt.Sprintf("toy firmware image number %d with padding to be a few bytes long", v))
}

func pkgN(v int) Package {
	return makePackage(priv, fmt.Sprintf("1.%d", v), uint64(v), int64(v*10), int64(v*10+5000), body(v))
}

func fresh(t *testing.T, upto int) *Disk {
	t.Helper()
	d := NewDisk()
	for v := 1; v <= upto; v++ {
		if err := Install(d, cfg, pkgN(v), int64(v*10+1)); err != nil {
			t.Fatalf("setup install %d: %v", v, err)
		}
	}
	return d
}

func mustBoot(t *testing.T, d *Disk, v int, ctx string) {
	t.Helper()
	img, err := Boot(d)
	if err != nil || !bytes.Equal(img.Body, body(v)) || img.Counter != uint64(v) {
		t.Fatalf("%s: Boot = counter %d, %v; want version %d", ctx, img.Counter, err, v)
	}
}

func hwOf(t *testing.T, d *Disk) HW {
	t.Helper()
	h, err := readHW(d)
	if err != nil {
		t.Fatalf("hw unreadable: %v", err)
	}
	return h
}

func TestVerifyAndErrors(t *testing.T) {
	st := State{}
	good := pkgN(1)
	if _, err := Verify(cfg, st, good, 11); err != nil {
		t.Fatalf("good package: %v", err)
	}
	_, other := key("some-other-key")
	if _, err := Verify(other, st, good, 11); !errors.Is(err, ErrBadSignature) {
		t.Errorf("wrong key: %v", err)
	}
	if _, err := Verify(Config{}, st, good, 11); !errors.Is(err, ErrBadSignature) {
		t.Errorf("no key must not panic: %v", err)
	}
	bad := good
	bad.Body = append([]byte("x"), good.Body[1:]...)
	if _, err := Verify(cfg, st, bad, 11); !errors.Is(err, ErrBodyMismatch) {
		t.Errorf("same size, edited body: %v", err)
	}
	bad.Body = good.Body[:len(good.Body)-1]
	if _, err := Verify(cfg, st, bad, 11); !errors.Is(err, ErrBodyMismatch) {
		t.Errorf("short body: %v", err)
	}
	bad = good
	bad.Meta = bytes.Replace(good.Meta, []byte("counter=1"), []byte("counter=9"), 1)
	if _, err := Verify(cfg, st, bad, 11); !errors.Is(err, ErrBadSignature) {
		t.Errorf("edited metadata: %v", err)
	}
	junk := []byte("not metadata\n")
	if _, err := Verify(cfg, st, Package{junk, ed25519.Sign(priv, junk), good.Body}, 11); !errors.Is(err, ErrBadMeta) {
		t.Errorf("signed junk: %v", err)
	}
	if _, err := Verify(cfg, st, good, 5000+10); !errors.Is(err, ErrExpired) {
		t.Errorf("expired: %v", err)
	}
	if _, err := Verify(cfg, State{Counter: 1, HighWater: 1, Digest: digestOf(body(1)), LastIssued: 10}, good, 11); !errors.Is(err, ErrAlreadyInst) {
		t.Errorf("replay: %v", err)
	}
	if _, err := Verify(cfg, State{Counter: 3, HighWater: 3, Digest: "x", LastIssued: 10}, good, 11); !errors.Is(err, ErrDowngrade) {
		t.Errorf("downgrade: %v", err)
	}
}

func TestInstallSequenceAndLayout(t *testing.T) {
	d := NewDisk()
	if _, err := Boot(d); !errors.Is(err, ErrNoImage) {
		t.Fatalf("blank device: %v", err)
	}
	if err := Install(d, cfg, pkgN(1), 11); err != nil {
		t.Fatal(err)
	}
	img, _ := Boot(d)
	if img.Slot != SlotA {
		t.Errorf("first install should use slot.a, used %s", img.Slot)
	}
	if err := Install(d, cfg, pkgN(2), 21); err != nil {
		t.Fatal(err)
	}
	img, _ = Boot(d)
	if img.Slot != SlotB || img.Version != "1.2" {
		t.Errorf("second install: %+v", img)
	}
	// The previous version must still be intact in the spare slot.
	if got, _ := d.Read(SlotA); !bytes.Equal(got, body(1)) {
		t.Error("the running slot's predecessor was overwritten")
	}
	if err := Install(d, cfg, pkgN(3), 31); err != nil {
		t.Fatal(err)
	}
	mustBoot(t, d, 3, "third")
	if h := hwOf(t, d); h.Counter != 3 || h.Issued != 30 {
		t.Errorf("hw = %+v", h)
	}
	if snap := d.Snapshot(); snap[ActiveFile+".tmp"] != "" || len(snap) != 4 {
		t.Errorf("stray files left behind: %v", reflect.ValueOf(snap).MapKeys())
	}
}

func TestRejectedPackagesWriteNothing(t *testing.T) {
	d := fresh(t, 2)
	before, ops := d.Snapshot(), d.Ops
	_, other := key("attacker")
	attackerPriv := ed25519.NewKeyFromSeed(append(make([]byte, 0, 32), bytes.Repeat([]byte{7}, 32)...))
	_ = other
	tampered := pkgN(3)
	tampered.Body = append([]byte{}, tampered.Body...)
	tampered.Body[0] ^= 1
	cases := map[string]struct {
		pkg  Package
		now  int64
		want error
	}{
		"old release":                       {pkgN(1), 100, ErrStaleMetadata},
		"downgrade with a fresh issue time": {makePackage(priv, "1.1", 1, 25, 5025, body(1)), 100, ErrDowngrade},
		"replay":                            {pkgN(2), 100, ErrAlreadyInst},
		"tampered":                          {tampered, 100, ErrBodyMismatch},
		"expired":                           {pkgN(3), 99999, ErrExpired},
		"future":                            {makePackage(priv, "9.9", 9, 1000000, 2000000, body(9)), 100, ErrFuture},
		"wrong signer":                      {makePackage(attackerPriv, "1.3", 3, 30, 5030, body(3)), 100, ErrBadSignature},
		"stale":                             {makePackage(priv, "1.3", 3, 5, 5030, body(3)), 100, ErrStaleMetadata},
		"equivocation":                      {makePackage(priv, "1.2", 2, 20, 5020, []byte("different content!")), 100, ErrEquivocation},
	}
	for name, c := range cases {
		if err := Install(d, cfg, c.pkg, c.now); !errors.Is(err, c.want) {
			t.Errorf("%s: got %v, want %v", name, err, c.want)
		}
		if d.Ops != ops || !reflect.DeepEqual(d.Snapshot(), before) {
			t.Errorf("%s: a rejected package touched the disk (%d -> %d mutating ops)", name, ops, d.Ops)
			ops, before = d.Ops, d.Snapshot()
		}
	}
	mustBoot(t, d, 2, "after rejections")
}

// installOps measures how many mutating operations a clean install of v takes on a device holding upto versions.
func installOps(t *testing.T, upto, v int) int {
	d := fresh(t, upto)
	start := d.Ops
	if err := Install(d, cfg, pkgN(v), 1000); err != nil {
		t.Fatal(err)
	}
	return d.Ops - start
}

func armed(d *Disk, faults map[int]Fault) {
	d.Plan = map[int]Fault{}
	for k, f := range faults {
		d.Plan[d.Ops+k] = f
	}
}

// afterFaults restarts the device and checks every property from the lesson.
func afterFaults(t *testing.T, d *Disk, upto, v int, installErr error, ctx string) {
	t.Helper()
	hw0 := hwOf2(d)
	d.Plan = nil
	d.Restart()
	if err := Recover(d); err != nil {
		t.Fatalf("%s: Recover: %v", ctx, err)
	}
	img, err := Boot(d)
	if upto == 0 && errors.Is(err, ErrNoImage) {
		if installErr == nil {
			t.Fatalf("%s: Install reported success but nothing boots", ctx)
		}
	} else {
		if err != nil {
			t.Fatalf("%s: Boot failed: %v", ctx, err)
		}
		okOld := upto > 0 && bytes.Equal(img.Body, body(upto)) && img.Counter == uint64(upto)
		okNew := bytes.Equal(img.Body, body(v)) && img.Counter == uint64(v)
		if !okOld && !okNew {
			t.Fatalf("%s: Boot returned a mixture: counter %d, %q", ctx, img.Counter, img.Body)
		}
		if installErr == nil && !okNew {
			t.Fatalf("%s: Install returned nil but the old version boots", ctx)
		}
		if hw := hwOf(t, d); hw.Counter < img.Counter {
			t.Fatalf("%s: high-water %d below booted counter %d", ctx, hw.Counter, img.Counter)
		}
	}
	if hw := hwOf(t, d); hw.Counter < hw0.Counter || hw.Issued < hw0.Issued {
		t.Fatalf("%s: high-water mark decreased from %+v to %+v", ctx, hw0, hw)
	}
	if err := Install(d, cfg, pkgN(v), 1000); err != nil && !errors.Is(err, ErrAlreadyInst) {
		t.Fatalf("%s: retry failed: %v", ctx, err)
	}
	mustBoot(t, d, v, ctx+" after retry")
	if hw := hwOf(t, d); hw.Counter != uint64(v) {
		t.Fatalf("%s: high-water %d after success, want %d", ctx, hw.Counter, v)
	}
	before, ops := d.Snapshot(), d.Ops
	if upto > 0 {
		if err := Install(d, cfg, makePackage(priv, "old", uint64(upto), int64(v*10), 99999, body(upto)), 1000); !errors.Is(err, ErrDowngrade) {
			t.Fatalf("%s: old package after update: %v", ctx, err)
		}
	}
	if d.Ops != ops || !reflect.DeepEqual(before, d.Snapshot()) {
		t.Fatalf("%s: refused downgrade modified the disk", ctx)
	}
}

func hwOf2(d *Disk) HW { h, _ := readHW(d); return h }

var allFaults = []Fault{FaultError, FaultTorn, FaultCrash, FaultTornCrash, FaultCrashAfter, FaultLie}

func TestEveryFaultAtEveryPosition(t *testing.T) {
	for _, upto := range []int{0, 1, 2} {
		v := upto + 1
		n := installOps(t, upto, v)
		for pos := 1; pos <= n+1; pos++ {
			for _, f := range allFaults {
				d := fresh(t, upto)
				armed(d, map[int]Fault{pos: f})
				err := Install(d, cfg, pkgN(v), 1000)
				afterFaults(t, d, upto, v, err, fmt.Sprintf("base=%d op %d/%d fault %d", upto, pos, n, f))
			}
		}
	}
}

func TestRandomFaultSchedules(t *testing.T) {
	for seed := int64(0); seed < 400; seed++ {
		rng := rand.New(rand.NewSource(seed))
		upto := rng.Intn(3)
		v := upto + 1
		n := installOps(t, upto, v) + 4
		d := fresh(t, upto)
		// Several attempts, each with its own faults, before one clean retry.
		var lastErr error
		for attempt := 0; attempt < 1+rng.Intn(3); attempt++ {
			faults := map[int]Fault{}
			for k := 0; k < 1+rng.Intn(3); k++ {
				faults[1+rng.Intn(n)] = allFaults[rng.Intn(len(allFaults))]
			}
			armed(d, faults)
			lastErr = Install(d, cfg, pkgN(v), 1000)
			d.Plan = nil
			d.Restart()
			if img, err := Boot(d); err == nil && !bytes.Equal(img.Body, body(v)) && (upto == 0 || !bytes.Equal(img.Body, body(upto))) {
				t.Fatalf("seed %d attempt %d: Boot returned a mixture", seed, attempt)
			}
		}
		afterFaults(t, d, upto, v, lastErr, fmt.Sprintf("seed %d", seed))
	}
}

func TestHealthRollback(t *testing.T) {
	d := fresh(t, 1)
	bad := cfg
	bad.Health = func(b []byte) bool { return !bytes.Equal(b, body(2)) }
	if err := Install(d, bad, pkgN(2), 100); !errors.Is(err, ErrUnhealthy) {
		t.Fatalf("unhealthy install: %v", err)
	}
	mustBoot(t, d, 1, "after rollback")
	if h := hwOf(t, d); h.Counter != 2 {
		t.Errorf("high-water mark %d; the abandoned counter 2 must stay burnt", h.Counter)
	}
	if err := Install(d, cfg, pkgN(2), 100); !errors.Is(err, ErrDowngrade) {
		t.Errorf("re-offering the version that was rolled back: %v", err)
	}
	fix := makePackage(priv, "1.2.1", 3, 25, 5025, []byte("repaired image three"))
	if err := Install(d, bad, fix, 100); err != nil {
		t.Fatalf("fixed release: %v", err)
	}
	img, _ := Boot(d)
	if string(img.Body) != "repaired image three" {
		t.Errorf("boots %q", img.Body)
	}
}

func TestHealthFailureOnFirstInstall(t *testing.T) {
	d := NewDisk()
	bad := cfg
	bad.Health = func([]byte) bool { return false }
	if err := Install(d, bad, pkgN(1), 100); !errors.Is(err, ErrUnhealthy) {
		t.Fatalf("got %v", err)
	}
	if _, err := Boot(d); !errors.Is(err, ErrNoImage) {
		t.Errorf("with nothing to roll back to, expected no image: %v", err)
	}
	if err := Install(d, cfg, pkgN(1), 100); !errors.Is(err, ErrDowngrade) {
		t.Errorf("the unhealthy counter must stay burnt: %v", err)
	}
	if err := Install(d, cfg, pkgN(2), 100); err != nil {
		t.Fatalf("next release: %v", err)
	}
	mustBoot(t, d, 2, "recovery")
}

func TestFaultsDuringHealthRollback(t *testing.T) {
	bad := cfg
	bad.Health = func([]byte) bool { return false }
	n := 0
	{
		d := fresh(t, 1)
		s := d.Ops
		Install(d, bad, pkgN(2), 100)
		n = d.Ops - s
	}
	for pos := 1; pos <= n; pos++ {
		for _, f := range allFaults {
			d := fresh(t, 1)
			armed(d, map[int]Fault{pos: f})
			Install(d, bad, pkgN(2), 100)
			d.Plan = nil
			d.Restart()
			if err := Recover(d); err != nil {
				t.Fatalf("pos %d fault %d: Recover: %v", pos, f, err)
			}
			img, err := Boot(d)
			if err != nil || (!bytes.Equal(img.Body, body(1)) && !bytes.Equal(img.Body, body(2))) {
				t.Fatalf("pos %d fault %d: Boot = %v, %q", pos, f, err, img.Body)
			}
			if h := hwOf(t, d); h.Counter < img.Counter {
				t.Fatalf("pos %d fault %d: hw %d below booted %d", pos, f, h.Counter, img.Counter)
			}
			// A later rollback attempt must still be possible and end on version 1.
			if err := Install(d, bad, pkgN(3), 100); err != nil && !errors.Is(err, ErrUnhealthy) {
				t.Fatalf("pos %d fault %d: next install: %v", pos, f, err)
			}
		}
	}
}

func TestBitRotFallsBack(t *testing.T) {
	d := fresh(t, 2)
	d.Corrupt(SlotB) // version 2 is in slot.b
	img, err := Boot(d)
	if err != nil || !bytes.Equal(img.Body, body(1)) {
		t.Fatalf("corrupt running slot: %v %q", err, img.Body)
	}
	if h := hwOf(t, d); h.Counter != 2 {
		t.Errorf("fall back must keep the high-water mark at 2, got %d", h.Counter)
	}
	mustBoot(t, d, 1, "second boot")
	if err := Install(d, cfg, pkgN(2), 100); !errors.Is(err, ErrDowngrade) {
		t.Errorf("rot must not reopen the corrupted version: %v", err)
	}

	d = fresh(t, 2)
	d.Corrupt(SlotB)
	d.Corrupt(SlotA)
	if _, err := Boot(d); !errors.Is(err, ErrNoImage) {
		t.Errorf("both slots damaged: %v", err)
	}

	d = fresh(t, 1)
	d.Corrupt(SlotA)
	if _, err := Boot(d); !errors.Is(err, ErrNoImage) {
		t.Errorf("only slot damaged: %v", err)
	}
}

func TestSpareSlotRotDoesNotMatter(t *testing.T) {
	d := fresh(t, 2)
	d.Corrupt(SlotA) // the previous version rots, the running one is fine
	mustBoot(t, d, 2, "running slot intact")
	if err := Install(d, cfg, pkgN(3), 100); err != nil {
		t.Fatalf("update over a rotted spare: %v", err)
	}
	mustBoot(t, d, 3, "after update")
}

func TestCorruptStateFailsClosed(t *testing.T) {
	d := fresh(t, 1)
	d.files[HWFile] = []byte("garbage")
	before, ops := d.Snapshot(), d.Ops
	if err := Install(d, cfg, pkgN(2), 100); !errors.Is(err, ErrCorruptState) {
		t.Errorf("corrupt high-water mark: %v", err)
	}
	if d.Ops != ops || !reflect.DeepEqual(before, d.Snapshot()) {
		t.Error("Install modified a device whose state it could not trust")
	}
	d = fresh(t, 1)
	d.files[ActiveFile] = []byte("cur nonsense")
	if _, err := Boot(d); !errors.Is(err, ErrCorruptState) {
		t.Errorf("corrupt active file: %v", err)
	}
}

func TestRecoverIsIdempotentAndQuietWhenClean(t *testing.T) {
	d := fresh(t, 2)
	ops := d.Ops
	for i := 0; i < 3; i++ {
		if err := Recover(d); err != nil {
			t.Fatal(err)
		}
	}
	if d.Ops != ops {
		t.Errorf("Recover on a clean device performed %d writes", d.Ops-ops)
	}
	d.files[ActiveFile+".tmp"] = []byte("half")
	d.files[HWFile+".tmp"] = []byte("1")
	if err := Recover(d); err != nil {
		t.Fatal(err)
	}
	if _, ok := d.Read(ActiveFile + ".tmp"); ok {
		t.Error("stray active.tmp not removed")
	}
	if _, ok := d.Read(HWFile + ".tmp"); ok {
		t.Error("stray hw.tmp not removed")
	}
	// A crash between commit and the high-water update is repaired.
	d = fresh(t, 1)
	n := installOps(t, 1, 2)
	armed(d, map[int]Fault{n - 1: FaultCrashAfter}) // the rename of the new active file
	Install(d, cfg, pkgN(2), 100)
	d.Plan = nil
	d.Restart()
	if err := Recover(d); err != nil {
		t.Fatal(err)
	}
	if h := hwOf(t, d); h.Counter < 2 {
		t.Errorf("Recover left hw at %d behind the committed counter", h.Counter)
	}
}
