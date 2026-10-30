package main

import (
	"bytes"
	"encoding/binary"
	"hash/crc32"
	"math/rand"
	"testing"
)

func packageFixture(p []byte, key byte) []byte {
	b := make([]byte, 11+len(p))
	copy(b, "BX01")
	binary.LittleEndian.PutUint16(b[4:6], uint16(len(p)))
	b[6] = key
	binary.LittleEndian.PutUint32(b[7:11], crc32.ChecksumIEEE(p))
	for i, v := range p {
		b[11+i] = v ^ key
	}
	return b
}
func TestPackageRoundTrips(t *testing.T) {
	rng := rand.New(rand.NewSource(929))
	for _, n := range []int{1, 13, 257, 4096, 8192} {
		for _, key := range []byte{1, 19, 128, 255} {
			plain := make([]byte, n)
			rng.Read(plain)
			saved := append([]byte{}, plain...)
			want := packageFixture(plain, key)
			got, err := Pack(plain, key)
			if err != nil || !bytes.Equal(got, want) || !bytes.Equal(plain, saved) {
				t.Fatal("pack", n, key, err)
			}
			decoded, err := Unpack(want)
			if err != nil || !bytes.Equal(decoded, plain) || !bytes.Equal(want, packageFixture(plain, key)) {
				t.Fatal("unpack", n, key, err)
			}
			decoded[0] ^= 1
			if !bytes.Equal(want, packageFixture(saved, key)) {
				t.Fatal("unpack aliases input")
			}
			got[11] ^= 1
			if !bytes.Equal(plain, saved) {
				t.Fatal("pack aliases input")
			}
		}
	}
}
func TestPackageRejectsDamage(t *testing.T) {
	good := packageFixture([]byte{1, 2, 0, 255}, 19)
	bad := [][]byte{nil, good[:10], good[:len(good)-1], append(append([]byte{}, good...), 0), packageFixture(nil, 19), packageFixture(make([]byte, 8193), 19)}
	for _, at := range []int{0, 4, 6, 7, 11} {
		b := append([]byte{}, good...)
		if at == 6 {
			b[at] = 0
		} else {
			b[at] ^= 1
		}
		bad = append(bad, b)
	}
	for i, b := range bad {
		if out, err := Unpack(b); err == nil || out != nil {
			t.Fatal("accepted corrupt envelope", i)
		}
	}
	for _, c := range []struct {
		p []byte
		k byte
	}{{nil, 1}, {[]byte{0}, 0}, {make([]byte, 8193), 1}} {
		if out, err := Pack(c.p, c.k); err == nil || out != nil {
			t.Fatal("invalid input accepted")
		}
	}
}
