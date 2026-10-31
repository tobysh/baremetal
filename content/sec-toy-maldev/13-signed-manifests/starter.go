package main

import (
	"crypto/ed25519"
	"errors"
	"fmt"
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
	return Manifest{}, nil
}

func main() {
	pub, priv, _ := ed25519.GenerateKey(nil)
	_ = priv
	p := Policy{Keys: map[string]ed25519.PublicKey{"lab": pub}, Allowed: map[string]bool{"read:note": true}, Now: 10}
	fmt.Println(Admit(Signed{Manifest: []byte("{}")}, []byte("payload"), p))
}
