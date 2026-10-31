package main

import (
	"crypto/ed25519"
	"encoding/hex"
	"errors"
	"fmt"
	"strconv"
	"strings"
)

var (
	ErrUnknownKey   = errors.New("unknown key id")
	ErrRevoked      = errors.New("key revoked")
	ErrWrongRole    = errors.New("key may not sign this")
	ErrKeyExpired   = errors.New("key outside its validity window")
	ErrBadSignature = errors.New("bad signature")
	ErrBadRotation  = errors.New("malformed rotation")
	ErrDuplicateKey = errors.New("key id already trusted")
)

type TrustedKey struct {
	ID                   string
	Role                 string // "root" or "release"
	Pub                  ed25519.PublicKey
	NotBefore, NotAfter int64
}

type Store struct {
	Keys    []TrustedKey
	Revoked map[string]bool
}

type Signed struct {
	KeyID   string
	Payload []byte
	Sig     []byte
}

// SigningInput is what a signer signs: a label, the key id and the payload.
func SigningInput(keyID string, payload []byte) []byte {
	out := []byte("bm-signed-v1\x00" + keyID + "\x00")
	return append(out, payload...)
}

func find(keys []TrustedKey, id string) *TrustedKey {
	for i := range keys {
		if keys[i].ID == id {
			return &keys[i]
		}
	}
	return nil
}

func (s *Store) Verify(env Signed, role string, now int64) error {
	k := find(s.Keys, env.KeyID)
	if k == nil {
		return ErrUnknownKey
	}
	if s.Revoked[k.ID] {
		return ErrRevoked
	}
	if k.Role != role {
		return ErrWrongRole
	}
	if now < k.NotBefore || now >= k.NotAfter {
		return ErrKeyExpired
	}
	if len(k.Pub) != ed25519.PublicKeySize || !ed25519.Verify(k.Pub, SigningInput(env.KeyID, env.Payload), env.Sig) {
		return ErrBadSignature
	}
	return nil
}

func (s *Store) ApplyRotation(env Signed, now int64) error {
	if err := s.Verify(env, "root", now); err != nil {
		return err
	}
	text := string(env.Payload)
	if text == "" || !strings.HasSuffix(text, "\n") {
		return ErrBadRotation
	}
	keys := append([]TrustedKey(nil), s.Keys...)
	revoked := map[string]bool{}
	for k, v := range s.Revoked {
		revoked[k] = v
	}
	for _, line := range strings.Split(strings.TrimSuffix(text, "\n"), "\n") {
		f := strings.Split(line, " ")
		switch {
		case f[0] == "add" && len(f) == 6:
			if f[2] != "root" && f[2] != "release" || f[1] == "" {
				return ErrBadRotation
			}
			nb, e1 := strconv.ParseInt(f[3], 10, 64)
			na, e2 := strconv.ParseInt(f[4], 10, 64)
			pk, e3 := hex.DecodeString(f[5])
			if e1 != nil || e2 != nil || e3 != nil || nb >= na || len(pk) != ed25519.PublicKeySize {
				return ErrBadRotation
			}
			if find(keys, f[1]) != nil {
				return ErrDuplicateKey
			}
			keys = append(keys, TrustedKey{f[1], f[2], pk, nb, na})
		case f[0] == "revoke" && len(f) == 2:
			if find(keys, f[1]) == nil {
				return ErrBadRotation
			}
			revoked[f[1]] = true
		default:
			return ErrBadRotation
		}
	}
	s.Keys, s.Revoked = keys, revoked
	return nil
}

func sign(priv ed25519.PrivateKey, id string, payload string) Signed {
	return Signed{id, []byte(payload), ed25519.Sign(priv, SigningInput(id, []byte(payload)))}
}

func seedKey(name string) ed25519.PrivateKey {
	seed := make([]byte, ed25519.SeedSize)
	copy(seed, name)
	return ed25519.NewKeyFromSeed(seed)
}

// Try it with Run. All keys are throwaway values derived from fixed seeds.
func main() {
	root, rel1, rel2 := seedKey("lesson3-root"), seedKey("lesson3-rel1"), seedKey("lesson3-rel2")
	pub := func(k ed25519.PrivateKey) ed25519.PublicKey { return k.Public().(ed25519.PublicKey) }
	s := &Store{Revoked: map[string]bool{}, Keys: []TrustedKey{
		{"root-1", "root", pub(root), 0, 1000},
		{"rel-1", "release", pub(rel1), 0, 500},
	}}
	fmt.Println("release signed by rel-1:", s.Verify(sign(rel1, "rel-1", "v1.0"), "release", 100))
	rot := fmt.Sprintf("add rel-2 release 400 900 %s\nrevoke rel-1\n", hex.EncodeToString(pub(rel2)))
	fmt.Println("rotation by release key:", s.ApplyRotation(sign(rel1, "rel-1", rot), 450))
	fmt.Println("rotation by root key:", s.ApplyRotation(sign(root, "root-1", rot), 450))
	fmt.Println("old key now:", s.Verify(sign(rel1, "rel-1", "v1.1"), "release", 450))
	fmt.Println("new key:", s.Verify(sign(rel2, "rel-2", "v1.1"), "release", 450))
}
