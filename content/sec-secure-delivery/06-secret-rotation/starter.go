package main

import (
	"crypto/subtle"
	"errors"
	"fmt"
	"math"
)

var (
	ErrWeak          = errors.New("secret too short")
	ErrReuse         = errors.New("secret was used before")
	ErrClock         = errors.New("clock moved backwards")
	ErrNoSuchVersion = errors.New("no such version")
)

type version struct {
	secret  string
	end     int64 // exclusive; math.MaxInt64 while current
	revoked bool
}

type KeyRing struct {
	grace      int64
	versions   []version
	lastRotate int64
}

var _ = subtle.ConstantTimeCompare
var _ = math.MaxInt64

func NewKeyRing(grace int64) *KeyRing { return nil }

func (k *KeyRing) Rotate(secret string, now int64) (int, error) { return 0, nil }
func (k *KeyRing) Revoke(v int) error                           { return nil }
func (k *KeyRing) Check(secret string, now int64) (int, bool)   { return 0, false }
func (k *KeyRing) Current() int                                 { return 0 }

// Try it with Run. All secrets are invented.
func main() {
	k := NewKeyRing(100)
	k.Rotate("first-secret-value-000", 0)
	k.Rotate("second-secret-value-111", 1000)
	for _, now := range []int64{1050, 1100, 1200} {
		v1, ok1 := k.Check("first-secret-value-000", now)
		v2, ok2 := k.Check("second-secret-value-111", now)
		fmt.Println("t =", now, "old:", v1, ok1, "new:", v2, ok2)
	}
	k.Revoke(2)
	fmt.Println(k.Check("second-secret-value-111", 1201))
}
