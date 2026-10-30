package main

import (
	"crypto/hmac"
	"crypto/sha256"
	"encoding/base64"
	"encoding/hex"
	"fmt"
	"testing"
)

func labSign(body, key string) string {
	m := hmac.New(sha256.New, []byte(key))
	m.Write([]byte(body))
	return base64.RawURLEncoding.EncodeToString([]byte(body)) + "." + hex.EncodeToString(m.Sum(nil))
}
func TestForgedCookieAccepted(t *testing.T) {
	for i := 0; i < 20; i++ {
		key := fmt.Sprint("training-secret-", i)
		body := fmt.Sprintf("user=u%d&role=reader", i)
		want := fmt.Sprintf("user=u%d&role=admin", i)
		known := labSign(body, key)
		got, err := ForgeCookie(known, []string{"wrong", key, "other"}, want)
		if err != nil || got != labSign(want, key) || got == known {
			t.Fatal("forged cookie rejected by target", i, err)
		}
	}
}
func TestCookieErrors(t *testing.T) {
	for _, s := range []string{"bad", "%%.00", "YQ.zz", labSign("guest", "absent")} {
		if _, err := ForgeCookie(s, []string{"wrong"}, "admin"); err == nil {
			t.Fatal("accepted", s)
		}
	}
}

func TestCandidatePositions(t *testing.T) {
	for pos := 0; pos < 5; pos++ {
		keys := []string{"a", "b", "c", "d", "e"}
		key := keys[pos]
		desired := fmt.Sprint("role=admin&nonce=", pos)
		got, err := ForgeCookie(labSign("role=guest", key), keys, desired)
		if err != nil || got != labSign(desired, key) {
			t.Fatal("candidate position", pos, got, err)
		}
	}
	if _, err := ForgeCookie(labSign("guest", "key"), nil, "admin"); err == nil {
		t.Fatal("empty dictionary accepted")
	}
}
