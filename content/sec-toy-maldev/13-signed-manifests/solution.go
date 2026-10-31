package main

import (
	"bytes"
	"crypto/ed25519"
	"crypto/sha256"
	"encoding/hex"
	"encoding/json"
	"errors"
	"fmt"
	"io"
)

var (
	ErrMalformed = errors.New("malformed manifest")
	ErrUntrusted = errors.New("untrusted signature")
	ErrDigest    = errors.New("payload digest mismatch")
	ErrExpired   = errors.New("manifest expired")
	ErrForbidden = errors.New("permission not allowed")
)

const Domain = "BM-MANIFEST-v1\x00"

type Manifest struct {
	Name     string   `json:"name"`
	Digest   string   `json:"digest"`
	Perms    []string `json:"perms"`
	KeyID    string   `json:"key_id"`
	NotAfter uint64   `json:"not_after"`
}
type Signed struct{ Manifest, Sig []byte }
type Policy struct {
	Keys    map[string]ed25519.PublicKey
	Allowed map[string]bool
	Now     uint64
}

func Admit(s Signed, payload []byte, p Policy) (Manifest, error) {
	fail := func(e error, why string) (Manifest, error) { return Manifest{}, fmt.Errorf("%w: %s", e, why) }
	var m Manifest
	dec := json.NewDecoder(bytes.NewReader(s.Manifest))
	dec.DisallowUnknownFields()
	if err := dec.Decode(&m); err != nil {
		return fail(ErrMalformed, "json")
	}
	if _, err := dec.Token(); err != io.EOF {
		return fail(ErrMalformed, "trailing data")
	}
	if len(m.Name) == 0 || len(m.Name) > 64 || m.KeyID == "" || len(m.Perms) > 8 || !lowerHex64(m.Digest) {
		return fail(ErrMalformed, "fields")
	}
	seen := map[string]bool{}
	for _, perm := range m.Perms {
		if perm == "" || seen[perm] {
			return fail(ErrMalformed, "permissions")
		}
		seen[perm] = true
	}
	pub, ok := p.Keys[m.KeyID]
	if !ok || len(pub) != ed25519.PublicKeySize || len(s.Sig) != ed25519.SignatureSize {
		return fail(ErrUntrusted, "key or signature shape")
	}
	msg := append([]byte(Domain), s.Manifest...)
	if !ed25519.Verify(pub, msg, s.Sig) {
		return fail(ErrUntrusted, "bad signature")
	}
	sum := sha256.Sum256(payload)
	if hex.EncodeToString(sum[:]) != m.Digest {
		return fail(ErrDigest, "payload differs from manifest")
	}
	if p.Now > m.NotAfter {
		return fail(ErrExpired, "past not_after")
	}
	for _, perm := range m.Perms {
		if !p.Allowed[perm] {
			return fail(ErrForbidden, perm)
		}
	}
	return m, nil
}

func lowerHex64(s string) bool {
	if len(s) != 64 {
		return false
	}
	for _, c := range s {
		if !(c >= '0' && c <= '9' || c >= 'a' && c <= 'f') {
			return false
		}
	}
	return true
}

func main() {
	pub, priv, _ := ed25519.GenerateKey(nil)
	_ = priv
	p := Policy{Keys: map[string]ed25519.PublicKey{"lab": pub}, Allowed: map[string]bool{"read:note": true}, Now: 10}
	fmt.Println(Admit(Signed{Manifest: []byte("{}")}, []byte("payload"), p))
}
