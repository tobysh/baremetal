package main

import (
	"errors"
	"testing"
)

// Independent reference: brute force, with its own copy of the feature table.
var refSince = [][2]uint32{{1 << 0, 1}, {1 << 1, 2}, {1 << 2, 3}}

func refNegotiate(a, b Hello) (ver uint8, feats uint32, kind string) {
	for _, h := range []Hello{a, b} {
		if h.MinVer == 0 || h.MinVer > h.MaxVer {
			return 0, 0, "bad"
		}
	}
	found := 0
	for v := 255; v >= 1; v-- {
		if v >= int(a.MinVer) && v <= int(a.MaxVer) && v >= int(b.MinVer) && v <= int(b.MaxVer) {
			found = v
			break
		}
	}
	if found == 0 {
		return 0, 0, "version"
	}
	for _, e := range refSince {
		if a.Features&e[0] != 0 && b.Features&e[0] != 0 && uint32(found) >= e[1] {
			feats |= e[0]
		}
	}
	for bit := 0; bit < 32; bit++ {
		m := uint32(1) << bit
		if (a.Required|b.Required)&m != 0 && feats&m == 0 {
			return 0, 0, "feature"
		}
	}
	return uint8(found), feats, ""
}

var sentinel = map[string]error{"bad": ErrBadHello, "version": ErrNoVersion, "feature": ErrMissingFeature}

func TestNegotiate_matches_brute_force(t *testing.T) {
	masks := []uint32{0, 1, 2, 4, 7, 1 << 9, 3 | 1<<9, 1 << 31}
	reqs := []uint32{0, 1, 2, 4, 1 << 9}
	count := 0
	for amin := uint8(0); amin <= 4; amin++ {
		for amax := uint8(0); amax <= 4; amax++ {
			for bmin := uint8(0); bmin <= 4; bmin++ {
				for bmax := uint8(0); bmax <= 4; bmax++ {
					for _, af := range masks {
						for _, bf := range masks {
							for _, ar := range reqs {
								br := reqs[(int(ar)+int(af))%len(reqs)]
								a := Hello{amin, amax, af, ar}
								b := Hello{bmin, bmax, bf, br}
								wv, wf, kind := refNegotiate(a, b)
								for _, pair := range [][2]Hello{{a, b}, {b, a}} {
									got, err := Negotiate(pair[0], pair[1])
									count++
									if kind != "" {
										if !errors.Is(err, sentinel[kind]) || got != (Agreement{}) {
											t.Fatalf("%+v vs %+v: got %+v, %v; want %v", pair[0], pair[1], got, err, sentinel[kind])
										}
									} else if err != nil || got.Version != wv || got.Features != wf {
										t.Fatalf("%+v vs %+v: got %+v, %v; want version %d features %b", pair[0], pair[1], got, err, wv, wf)
									}
								}
							}
						}
					}
				}
			}
		}
	}
	if count < 1000 {
		t.Fatal("test did not run")
	}
}

func TestNegotiate_documented_examples(t *testing.T) {
	newer := Hello{1, 3, FeatAck | FeatCompress | FeatBatch | 1<<9, 0}
	older := Hello{1, 2, FeatAck | FeatCompress, 0}
	got, err := Negotiate(newer, older)
	if err != nil || got != (Agreement{2, FeatAck | FeatCompress}) {
		t.Errorf("newer vs older: %+v, %v", got, err)
	}
	// Batch offered by both but the agreed version predates it.
	a := Hello{1, 2, FeatBatch, 0}
	got, err = Negotiate(a, a)
	if err != nil || got.Features != 0 {
		t.Errorf("feature from the future enabled: %+v, %v", got, err)
	}
	// Unknown required bit can never be satisfied, even if both set it.
	u := Hello{1, 1, 1 << 9, 1 << 9}
	if _, err := Negotiate(u, u); !errors.Is(err, ErrMissingFeature) {
		t.Errorf("unknown required bit: %v", err)
	}
}

func TestHello_round_trip_and_layout(t *testing.T) {
	h := Hello{2, 5, 0x01020304, 0x0A0B0C0D}
	b := EncodeHello(h)
	want := []byte{2, 5, 1, 2, 3, 4, 0x0A, 0x0B, 0x0C, 0x0D}
	if string(b) != string(want) {
		t.Fatalf("EncodeHello = % x; want % x", b, want)
	}
	got, err := DecodeHello(b)
	if err != nil || got != h {
		t.Errorf("round trip: %+v, %v", got, err)
	}
}

func TestDecodeHello_compatibility(t *testing.T) {
	legacy := []byte{1, 2, 0, 0, 0, 3}
	h, err := DecodeHello(legacy)
	if err != nil || h != (Hello{1, 2, 3, 0}) {
		t.Errorf("v1 six-byte hello: %+v, %v", h, err)
	}
	future := append(EncodeHello(Hello{1, 4, 7, 1}), 0xDE, 0xAD, 0xBE, 0xEF, 9)
	h, err = DecodeHello(future)
	if err != nil || h != (Hello{1, 4, 7, 1}) {
		t.Errorf("hello with trailing extension bytes: %+v, %v", h, err)
	}
	// 7-9 bytes: the Required field is incomplete, so it is treated as absent.
	h, err = DecodeHello([]byte{1, 2, 0, 0, 0, 3, 0, 0, 1})
	if err != nil || h.Required != 0 {
		t.Errorf("partial Required field: %+v, %v", h, err)
	}
	for n := 0; n < 6; n++ {
		if _, err := DecodeHello(make([]byte, n)); !errors.Is(err, ErrBadHello) {
			t.Errorf("%d byte hello: %v; want ErrBadHello", n, err)
		}
	}
}

func TestDecodeHello_does_not_alias(t *testing.T) {
	b := EncodeHello(Hello{1, 2, 3, 4})
	h, _ := DecodeHello(b)
	b[0] = 99
	if h.MinVer != 1 {
		t.Errorf("hello changed with the buffer")
	}
}
