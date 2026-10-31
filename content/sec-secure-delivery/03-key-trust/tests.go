package main

import (
	"crypto/ed25519"
	"encoding/hex"
	"errors"
	"fmt"
	"reflect"
	"testing"
)

func pk(k ed25519.PrivateKey) ed25519.PublicKey { return k.Public().(ed25519.PublicKey) }

type world struct {
	root, rel, rel2 ed25519.PrivateKey
	s               *Store
}

func newWorld() world {
	w := world{root: seedKey("t3-root"), rel: seedKey("t3-rel"), rel2: seedKey("t3-rel2")}
	w.s = &Store{Revoked: map[string]bool{}, Keys: []TrustedKey{
		{"root-1", "root", pk(w.root), 100, 1000},
		{"rel-1", "release", pk(w.rel), 200, 300},
	}}
	return w
}

func snapshot(s *Store) *Store {
	c := &Store{Revoked: map[string]bool{}, Keys: append([]TrustedKey(nil), s.Keys...)}
	for k, v := range s.Revoked {
		c.Revoked[k] = v
	}
	return c
}

func same(a, b *Store) bool {
	if len(a.Keys) != len(b.Keys) {
		return false
	}
	for i := range a.Keys {
		x, y := a.Keys[i], b.Keys[i]
		if x.ID != y.ID || x.Role != y.Role || x.NotBefore != y.NotBefore || x.NotAfter != y.NotAfter || !x.Pub.Equal(y.Pub) {
			return false
		}
	}
	ra, rb := map[string]bool{}, map[string]bool{}
	for k, v := range a.Revoked {
		if v {
			ra[k] = true
		}
	}
	for k, v := range b.Revoked {
		if v {
			rb[k] = true
		}
	}
	return reflect.DeepEqual(ra, rb)
}

func TestVerifyWindowAndRole(t *testing.T) {
	w := newWorld()
	env := sign(w.rel, "rel-1", "payload")
	cases := []struct {
		now  int64
		role string
		want error
	}{
		{199, "release", ErrKeyExpired}, {200, "release", nil}, {299, "release", nil},
		{300, "release", ErrKeyExpired}, {10000, "release", ErrKeyExpired},
		{250, "root", ErrWrongRole}, {250, "", ErrWrongRole}, {-5, "release", ErrKeyExpired},
	}
	for _, c := range cases {
		if err := w.s.Verify(env, c.role, c.now); !errors.Is(err, c.want) && !(c.want == nil && err == nil) {
			t.Errorf("now=%d role=%q: got %v, want %v", c.now, c.role, err, c.want)
		}
	}
}

func TestVerifySignatureProblems(t *testing.T) {
	w := newWorld()
	env := sign(w.rel, "rel-1", "payload")
	if err := w.s.Verify(Signed{"nobody", env.Payload, env.Sig}, "release", 250); !errors.Is(err, ErrUnknownKey) {
		t.Errorf("unknown id: %v", err)
	}
	tamper := env
	tamper.Payload = []byte("payloae")
	if err := w.s.Verify(tamper, "release", 250); !errors.Is(err, ErrBadSignature) {
		t.Errorf("tampered payload: %v", err)
	}
	// signed by root's key but claiming to be rel-1
	imp := sign(w.root, "rel-1", "payload")
	if err := w.s.Verify(imp, "release", 250); !errors.Is(err, ErrBadSignature) {
		t.Errorf("wrong private key under rel-1's id: %v", err)
	}
	// A signature made under one id must not verify as another id with the same key.
	w.s.Keys = append(w.s.Keys, TrustedKey{"rel-alias", "release", pk(w.rel), 200, 300})
	if err := w.s.Verify(Signed{"rel-alias", env.Payload, env.Sig}, "release", 250); !errors.Is(err, ErrBadSignature) {
		t.Errorf("signature relabelled with another key id: %v", err)
	}
	if err := w.s.Verify(Signed{"rel-1", env.Payload, nil}, "release", 250); !errors.Is(err, ErrBadSignature) {
		t.Errorf("nil signature: %v", err)
	}
	w.s.Keys[1].Pub = w.s.Keys[1].Pub[:5]
	if err := w.s.Verify(env, "release", 250); !errors.Is(err, ErrBadSignature) {
		t.Errorf("short stored key must not panic: %v", err)
	}
}

func TestPolicyOrderBeforeCrypto(t *testing.T) {
	w := newWorld()
	bad := Signed{"rel-1", []byte("x"), []byte("not a signature")}
	if err := w.s.Verify(bad, "root", 250); !errors.Is(err, ErrWrongRole) {
		t.Errorf("wrong role with junk signature: %v", err)
	}
	if err := w.s.Verify(bad, "release", 999); !errors.Is(err, ErrKeyExpired) {
		t.Errorf("expired with junk signature: %v", err)
	}
	w.s.Revoked["rel-1"] = true
	good := sign(w.rel, "rel-1", "x")
	if err := w.s.Verify(good, "root", 999); !errors.Is(err, ErrRevoked) {
		t.Errorf("revoked beats role and window: %v", err)
	}
}

func addLine(id, role string, nb, na int64, p ed25519.PublicKey) string {
	return fmt.Sprintf("add %s %s %d %d %s\n", id, role, nb, na, hex.EncodeToString(p))
}

func TestRotationByRoot(t *testing.T) {
	w := newWorld()
	rot := addLine("rel-2", "release", 250, 900, pk(w.rel2)) + "revoke rel-1\n"
	if err := w.s.ApplyRotation(sign(w.root, "root-1", rot), 260); err != nil {
		t.Fatalf("rotation by root: %v", err)
	}
	if err := w.s.Verify(sign(w.rel, "rel-1", "x"), "release", 260); !errors.Is(err, ErrRevoked) {
		t.Errorf("old key after revoke: %v", err)
	}
	if err := w.s.Verify(sign(w.rel2, "rel-2", "x"), "release", 260); err != nil {
		t.Errorf("new key: %v", err)
	}
	if err := w.s.Verify(sign(w.rel2, "rel-2", "x"), "release", 249); !errors.Is(err, ErrKeyExpired) {
		t.Errorf("new key before its window: %v", err)
	}
	// root can add another root, and revoke itself via a second root
	r3 := seedKey("t3-root2")
	if err := w.s.ApplyRotation(sign(w.root, "root-1", addLine("root-2", "root", 0, 5000, pk(r3))), 500); err != nil {
		t.Fatalf("add root: %v", err)
	}
	if err := w.s.ApplyRotation(sign(r3, "root-2", "revoke root-1\n"), 600); err != nil {
		t.Fatalf("second root revokes first: %v", err)
	}
	if err := w.s.ApplyRotation(sign(w.root, "root-1", "revoke rel-2\n"), 600); !errors.Is(err, ErrRevoked) {
		t.Errorf("revoked root must not rotate: %v", err)
	}
}

func TestReleaseKeyCannotRotate(t *testing.T) {
	w := newWorld()
	before := snapshot(w.s)
	evil := seedKey("t3-evil")
	rot := addLine("evil", "root", 0, 99999, pk(evil))
	if err := w.s.ApplyRotation(sign(w.rel, "rel-1", rot), 250); !errors.Is(err, ErrWrongRole) {
		t.Errorf("release key rotating: %v", err)
	}
	if err := w.s.ApplyRotation(sign(evil, "evil", rot), 250); !errors.Is(err, ErrUnknownKey) {
		t.Errorf("self-introduced key: %v", err)
	}
	if err := w.s.ApplyRotation(sign(w.root, "root-1", rot), 5000); !errors.Is(err, ErrKeyExpired) {
		t.Errorf("expired root: %v", err)
	}
	if !same(before, w.s) {
		t.Error("a refused rotation changed the store")
	}
}

func TestRotationIsAtomic(t *testing.T) {
	w := newWorld()
	good := addLine("rel-2", "release", 250, 900, pk(w.rel2))
	hexk := hex.EncodeToString(pk(w.rel2))
	bads := map[string]struct {
		payload string
		want    error
	}{
		"unknown verb":    {good + "frobnicate x\n", ErrBadRotation},
		"bad role":        {good + "add k3 admin 1 2 " + hexk + "\n", ErrBadRotation},
		"bad number":      {good + "add k3 release x 2 " + hexk + "\n", ErrBadRotation},
		"empty window":    {good + "add k3 release 5 5 " + hexk + "\n", ErrBadRotation},
		"short key":       {good + "add k3 release 1 2 abcd\n", ErrBadRotation},
		"not hex":         {good + "add k3 release 1 2 " + hexk[:62] + "zz\n", ErrBadRotation},
		"revoke unknown":  {good + "revoke ghost\n", ErrBadRotation},
		"no newline":      {good + "revoke rel-1", ErrBadRotation},
		"empty":           {"", ErrBadRotation},
		"blank line":      {good + "\nrevoke rel-1\n", ErrBadRotation},
		"duplicate id":    {good + "revoke rel-1\n" + addLine("rel-1", "release", 1, 2, pk(w.rel2)), ErrDuplicateKey},
		"duplicate twice": {good + good, ErrDuplicateKey},
	}
	for name, b := range bads {
		w2 := newWorld()
		before := snapshot(w2.s)
		err := w2.s.ApplyRotation(sign(w2.root, "root-1", b.payload), 260)
		if !errors.Is(err, b.want) {
			t.Errorf("%s: got %v, want %v", name, err, b.want)
		}
		if !same(before, w2.s) {
			t.Errorf("%s: store changed although the rotation failed", name)
		}
	}
	// revoking a key added earlier in the same payload is fine
	if err := w.s.ApplyRotation(sign(w.root, "root-1", good+"revoke rel-2\n"), 260); err != nil {
		t.Errorf("add then revoke in one payload: %v", err)
	}
	if err := w.s.Verify(sign(w.rel2, "rel-2", "x"), "release", 300); !errors.Is(err, ErrRevoked) {
		t.Errorf("added-and-revoked key: %v", err)
	}
}
