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

func NewKeyRing(grace int64) *KeyRing {
	if grace < 0 {
		grace = 0
	}
	return &KeyRing{grace: grace}
}

func (k *KeyRing) Rotate(secret string, now int64) (int, error) {
	if len(secret) < 16 {
		return 0, ErrWeak
	}
	for _, v := range k.versions {
		if subtle.ConstantTimeCompare([]byte(v.secret), []byte(secret)) == 1 {
			return 0, ErrReuse
		}
	}
	if len(k.versions) > 0 && now < k.lastRotate {
		return 0, ErrClock
	}
	if n := len(k.versions); n > 0 {
		end := int64(math.MaxInt64)
		if now <= math.MaxInt64-k.grace {
			end = now + k.grace
		}
		if end < k.versions[n-1].end {
			k.versions[n-1].end = end
		}
	}
	k.versions = append(k.versions, version{secret: secret, end: math.MaxInt64})
	k.lastRotate = now
	return len(k.versions), nil
}

func (k *KeyRing) Revoke(v int) error {
	if v < 1 || v > len(k.versions) {
		return ErrNoSuchVersion
	}
	k.versions[v-1].revoked = true
	return nil
}

func (k *KeyRing) Check(secret string, now int64) (int, bool) {
	best := 0
	for i, v := range k.versions {
		match := subtle.ConstantTimeCompare([]byte(v.secret), []byte(secret)) == 1
		if match && !v.revoked && now < v.end {
			best = i + 1
		}
	}
	return best, best > 0
}

func (k *KeyRing) Current() int { return len(k.versions) }

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
