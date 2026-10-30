package main

import (
	"bytes"
	"crypto/sha256"
	"encoding/binary"
	"encoding/hex"
	"fmt"
	"hash/crc32"
	"reflect"
	"strings"
	"testing"
)

func admissionCapsule(code []byte, entry uint16) []byte {
	b := make([]byte, 12+len(code))
	copy(b, "BML1")
	binary.LittleEndian.PutUint16(b[4:6], entry)
	binary.LittleEndian.PutUint16(b[6:8], uint16(len(code)))
	binary.LittleEndian.PutUint32(b[8:12], crc32.ChecksumIEEE(code))
	copy(b[12:], code)
	return b
}
func admissionPackage(plain []byte, key byte) []byte {
	b := make([]byte, 11+len(plain))
	copy(b, "BX01")
	binary.LittleEndian.PutUint16(b[4:6], uint16(len(plain)))
	b[6] = key
	binary.LittleEndian.PutUint32(b[7:11], crc32.ChecksumIEEE(plain))
	for i, v := range plain {
		b[11+i] = v ^ key
	}
	return b
}
func admissionDigest(b []byte) string { sum := sha256.Sum256(b); return hex.EncodeToString(sum[:]) }
func TestAdmissionIdentityAcrossEnvelopes(t *testing.T) {
	for i := 0; i < 16; i++ {
		text := fmt.Sprint("approved-", i)
		code := []byte{1, byte(len(text))}
		code = append(code, []byte(text)...)
		code = append(code, 0)
		capsule := admissionCapsule(code, 0)
		digest := admissionDigest(capsule)
		for _, key := range []byte{1, 127, 255} {
			bundle := admissionPackage(capsule, key)
			before := append([]byte{}, bundle...)
			got, err := AdmitBundle(bundle, nil, digest, 2, 0, len(text))
			if err != nil || !reflect.DeepEqual(got, Trace{Outputs: []string{text}, Reads: []string{}}) || !bytes.Equal(bundle, before) {
				t.Fatal(got, err)
			}
		}
	}
}
func TestAdmissionRejectsChangedArtifact(t *testing.T) {
	old := admissionCapsule([]byte{1, 1, 'a', 0}, 0)
	changed := admissionCapsule([]byte{1, 1, 'b', 0}, 0)
	for _, digest := range []string{admissionDigest(old), "", strings.ToUpper(admissionDigest(changed)), admissionDigest(admissionPackage(changed, 7))} {
		got, err := AdmitBundle(admissionPackage(changed, 7), nil, digest, 2, 0, 100)
		if err == nil || !reflect.DeepEqual(got, Trace{}) {
			t.Fatal("untrusted identity accepted")
		}
	}
}
func TestAdmissionAuditsSkippedRead(t *testing.T) {
	code := []byte{2, 6, 's', 'e', 'c', 'r', 'e', 't', 1, 2, 'o', 'k', 0}
	capsule := admissionCapsule(code, 8)
	bundle := admissionPackage(capsule, 19)
	digest := admissionDigest(capsule)
	if got, err := AdmitBundle(bundle, nil, digest, 3, 0, 100); err == nil || !reflect.DeepEqual(got, Trace{}) {
		t.Fatal("skipped capability ignored")
	}
	caps := map[string]string{"secret": "known"}
	got, err := AdmitBundle(bundle, caps, digest, 3, 0, 7)
	want := Trace{Outputs: []string{"ok"}, Reads: []string{}}
	if err != nil || !reflect.DeepEqual(got, want) || len(caps) != 1 || caps["secret"] != "known" {
		t.Fatal(got, err)
	}
	if got, err := AdmitBundle(bundle, caps, digest, 3, 0, 6); err == nil || !reflect.DeepEqual(got, Trace{}) {
		t.Fatal("skipped output not counted")
	}
}
func TestAdmissionWholeImageBudgets(t *testing.T) {
	capsule := admissionCapsule([]byte{3, 9, 0, 1, 2, 'o', 'k', 0}, 3)
	bundle := admissionPackage(capsule, 21)
	digest := admissionDigest(capsule)
	for _, c := range []struct {
		ops    int
		ticks  uint32
		output int
	}{{2, 9, 2}, {3, 8, 2}, {3, 9, 1}, {3, 9, -1}} {
		got, err := AdmitBundle(bundle, nil, digest, c.ops, c.ticks, c.output)
		if err == nil || !reflect.DeepEqual(got, Trace{}) {
			t.Fatal("prefix budget not enforced", c)
		}
	}
	got, err := AdmitBundle(bundle, nil, digest, 3, 9, 2)
	if err != nil || !reflect.DeepEqual(got, Trace{Outputs: []string{"ok"}, Reads: []string{}, Ticks: 0}) {
		t.Fatal(got, err)
	}
}
func TestAdmissionValidatesInnerAndOuterLayers(t *testing.T) {
	for _, capsule := range [][]byte{[]byte("junk"), admissionCapsule([]byte{99, 0}, 0), admissionCapsule([]byte{1, 1, 'x', 0}, 2)} {
		bundle := admissionPackage(capsule, 9)
		if got, err := AdmitBundle(bundle, nil, admissionDigest(capsule), 64, 100, 4096); err == nil || !reflect.DeepEqual(got, Trace{}) {
			t.Fatal("invalid inner image")
		}
	}
	if got, err := AdmitBundle([]byte("junk"), nil, "", 64, 100, 4096); err == nil || !reflect.DeepEqual(got, Trace{}) {
		t.Fatal("invalid envelope")
	}
}
func TestAdmissionZeroOutput(t *testing.T) {
	capsule := admissionCapsule([]byte{0}, 0)
	got, err := AdmitBundle(admissionPackage(capsule, 1), nil, admissionDigest(capsule), 1, 0, 0)
	if err != nil || !reflect.DeepEqual(got, Trace{Outputs: []string{}, Reads: []string{}}) {
		t.Fatal(got, err)
	}
}
