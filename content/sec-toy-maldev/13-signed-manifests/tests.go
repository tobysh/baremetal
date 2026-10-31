package main

import (
	"bytes"
	"crypto/ed25519"
	"crypto/sha256"
	"encoding/hex"
	"errors"
	"fmt"
	"testing"
)

func tkey(n byte) (ed25519.PublicKey, ed25519.PrivateKey) {
	seed := bytes.Repeat([]byte{n}, ed25519.SeedSize)
	priv := ed25519.NewKeyFromSeed(seed)
	return priv.Public().(ed25519.PublicKey), priv
}
func tdigest(b []byte) string { s := sha256.Sum256(b); return hex.EncodeToString(s[:]) }
func tsign(priv ed25519.PrivateKey, manifest string) Signed {
	return Signed{Manifest: []byte(manifest), Sig: ed25519.Sign(priv, append([]byte("BM-MANIFEST-v1\x00"), manifest...))}
}
func tmanifest(payload []byte, perms string, keyID string, notAfter int) string {
	return fmt.Sprintf(`{"name":"demo","digest":"%s","perms":[%s],"key_id":"%s","not_after":%d}`, tdigest(payload), perms, keyID, notAfter)
}
func tpolicy() Policy {
	pub1, _ := tkey(1)
	pub2, _ := tkey(2)
	return Policy{Keys: map[string]ed25519.PublicKey{"lab": pub1, "other": pub2}, Allowed: map[string]bool{"read:note": true, "emit": true, "deny": false}, Now: 100}
}
func want(t *testing.T, err, target error, what string) {
	t.Helper()
	if !errors.Is(err, target) {
		t.Fatal(what, "got", err)
	}
}

func TestAdmitAcceptsGenuinePermittedManifest(t *testing.T) {
	_, priv := tkey(1)
	for i := 0; i < 6; i++ {
		payload := []byte(fmt.Sprint("payload\x00", i))
		raw := tmanifest(payload, `"read:note","emit"`, "lab", 100+i)
		m, err := Admit(tsign(priv, raw), payload, tpolicy())
		if err != nil || m.Name != "demo" || m.Digest != tdigest(payload) || len(m.Perms) != 2 || m.NotAfter != uint64(100+i) {
			t.Fatal(m, err)
		}
	}
}

func TestChangedManifestInvalidatesSignature(t *testing.T) {
	_, priv := tkey(1)
	payload := []byte("data")
	signed := tsign(priv, tmanifest(payload, `"read:note"`, "lab", 500))
	for _, edit := range [][2]string{{"read:note", "read:nope"}, {"500", "900"}, {"demo", "demx"}, {`"perms"`, `"perms" `}} {
		forged := Signed{Manifest: bytes.Replace(signed.Manifest, []byte(edit[0]), []byte(edit[1]), 1), Sig: signed.Sig}
		m, err := Admit(forged, payload, tpolicy())
		want(t, err, ErrUntrusted, "edit "+edit[0])
		if m.Name != "" {
			t.Fatal("manifest returned on failure")
		}
	}
	flipped := append([]byte{}, signed.Sig...)
	flipped[10] ^= 1
	_, err := Admit(Signed{signed.Manifest, flipped}, payload, tpolicy())
	want(t, err, ErrUntrusted, "flipped signature bit")
}

func TestWrongKeysAndSignatureShapes(t *testing.T) {
	_, priv1 := tkey(1)
	_, priv3 := tkey(3)
	payload := []byte("data")
	raw := tmanifest(payload, `"emit"`, "lab", 500)
	signed := tsign(priv1, raw)
	// Signed by an untrusted private key but claiming the trusted key id.
	_, err := Admit(tsign(priv3, raw), payload, tpolicy())
	want(t, err, ErrUntrusted, "wrong private key")
	// Signature from a different trusted key than the one named.
	raw2 := tmanifest(payload, `"emit"`, "other", 500)
	_, err = Admit(tsign(priv1, raw2), payload, tpolicy())
	want(t, err, ErrUntrusted, "key id points at a different key")
	// Unknown key id.
	_, err = Admit(tsign(priv1, tmanifest(payload, `"emit"`, "ghost", 500)), payload, tpolicy())
	want(t, err, ErrUntrusted, "unknown key id")
	// Truncated and oversized signatures, and a malformed policy key, must not panic.
	for _, sig := range [][]byte{nil, signed.Sig[:63], append(append([]byte{}, signed.Sig...), 0)} {
		_, err = Admit(Signed{signed.Manifest, sig}, payload, tpolicy())
		want(t, err, ErrUntrusted, "signature shape")
	}
	p := tpolicy()
	p.Keys["lab"] = p.Keys["lab"][:10]
	_, err = Admit(signed, payload, p)
	want(t, err, ErrUntrusted, "short public key")
	// Signature made without the domain prefix.
	bare := Signed{[]byte(raw), ed25519.Sign(priv1, []byte(raw))}
	_, err = Admit(bare, payload, tpolicy())
	want(t, err, ErrUntrusted, "missing domain separation")
}

func TestValidAuthenticAndPermittedAreDifferentVerdicts(t *testing.T) {
	_, priv := tkey(1)
	payload := []byte("artifact")
	// Authentic but not permitted.
	_, err := Admit(tsign(priv, tmanifest(payload, `"read:note","deny"`, "lab", 500)), payload, tpolicy())
	want(t, err, ErrForbidden, "disallowed permission")
	_, err = Admit(tsign(priv, tmanifest(payload, `"never-listed"`, "lab", 500)), payload, tpolicy())
	want(t, err, ErrForbidden, "unlisted permission")
	// Authentic but expired (expiry equal to Now is still fine).
	_, err = Admit(tsign(priv, tmanifest(payload, `"emit"`, "lab", 99)), payload, tpolicy())
	want(t, err, ErrExpired, "expired")
	if _, err = Admit(tsign(priv, tmanifest(payload, `"emit"`, "lab", 100)), payload, tpolicy()); err != nil {
		t.Fatal("not_after == now must be accepted", err)
	}
	// Authentic but for another artifact.
	_, err = Admit(tsign(priv, tmanifest(payload, `"emit"`, "lab", 500)), []byte("artifacT"), tpolicy())
	want(t, err, ErrDigest, "digest")
	// Valid in shape but unsigned: untrusted, not forbidden, even with a bad permission.
	_, err = Admit(Signed{Manifest: []byte(tmanifest(payload, `"deny"`, "lab", 500))}, payload, tpolicy())
	want(t, err, ErrUntrusted, "unsigned")
	// Signed garbage is malformed, not accepted.
	for i, raw := range []string{
		`not json`, ``, `{}`,
		tmanifest(payload, `"emit"`, "lab", 500) + `{}`,
		`{"name":"demo","digest":"` + tdigest(payload) + `","perms":[],"key_id":"lab","not_after":5,"extra":1}`,
		`{"name":"demo","digest":"` + tdigest(payload)[:63] + `","perms":[],"key_id":"lab","not_after":5}`,
		`{"name":"demo","digest":"` + string(bytes.ToUpper([]byte(tdigest(payload)))) + `","perms":[],"key_id":"lab","not_after":5}`,
		tmanifest(payload, `"emit","emit"`, "lab", 500),
		tmanifest(payload, `""`, "lab", 500),
		tmanifest(payload, `"a","b","c","d","e","f","g","h","i"`, "lab", 500),
		tmanifest(payload, `"emit"`, "", 500),
		`{"name":"","digest":"` + tdigest(payload) + `","perms":[],"key_id":"lab","not_after":5}`,
	} {
		_, err = Admit(tsign(priv, raw), payload, tpolicy())
		if !errors.Is(err, ErrMalformed) {
			t.Fatal("malformed manifest case", i, err)
		}
	}
}

func TestAdmitDoesNotModifyInputs(t *testing.T) {
	_, priv := tkey(1)
	payload := []byte("keep me")
	s := tsign(priv, tmanifest(payload, `"emit"`, "lab", 500))
	m0, s0, p0 := append([]byte{}, s.Manifest...), append([]byte{}, s.Sig...), append([]byte{}, payload...)
	for i := 0; i < 3; i++ {
		if _, err := Admit(s, payload, tpolicy()); err != nil {
			t.Fatal(err)
		}
	}
	if !bytes.Equal(m0, s.Manifest) || !bytes.Equal(s0, s.Sig) || !bytes.Equal(p0, payload) || cap(s.Manifest) != cap(s.Manifest[:len(s.Manifest)]) {
		t.Fatal("inputs modified")
	}
}
