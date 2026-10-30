package main

import (
	"crypto/hmac"
	"crypto/sha256"
	"encoding/base64"
	"encoding/hex"
	"fmt"
	"strings"
)

func ForgeCookie(known string, candidates []string, desired string) (string, error) {
	parts := strings.Split(known, ".")
	if len(parts) != 2 {
		return "", fmt.Errorf("bad cookie")
	}
	body, err := base64.RawURLEncoding.DecodeString(parts[0])
	if err != nil {
		return "", err
	}
	sig, err := hex.DecodeString(parts[1])
	if err != nil {
		return "", err
	}
	for _, key := range candidates {
		mac := hmac.New(sha256.New, []byte(key))
		mac.Write(body)
		if !hmac.Equal(mac.Sum(nil), sig) {
			continue
		}
		mac = hmac.New(sha256.New, []byte(key))
		mac.Write([]byte(desired))
		return base64.RawURLEncoding.EncodeToString([]byte(desired)) + "." + hex.EncodeToString(mac.Sum(nil)), nil
	}
	return "", fmt.Errorf("key not found")
}

func main() {}
