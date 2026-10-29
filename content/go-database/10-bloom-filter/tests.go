package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"testing"
)

func key(prefix string, i int) []byte { return []byte(fmt.Sprintf("%s-%d", prefix, i)) }

func TestNoFalseNegatives(t *testing.T) {
	b := NewBloom(10000, 10)
	for i := 0; i < 10000; i++ {
		b.Add(key("present", i))
	}
	for i := 0; i < 10000; i++ {
		if !b.MayContain(key("present", i)) {
			t.Fatalf("key %d was added but MayContain says false", i)
		}
	}
}

func TestFalse_positive_rate_is_low(t *testing.T) {
	b := NewBloom(10000, 10)
	for i := 0; i < 10000; i++ {
		b.Add(key("present", i))
	}
	fp := 0
	for i := 0; i < 20000; i++ {
		if b.MayContain(key("absent", i)) {
			fp++
		}
	}
	if rate := float64(fp) / 20000; rate > 0.03 {
		t.Errorf("false-positive rate %.1f%% with 10 bits per key; want under 3%%", rate*100)
	}
	if fp == 20000 {
		t.Errorf("MayContain says true for everything")
	}
}

func TestEmpty_filter_contains_nothing(t *testing.T) {
	b := NewBloom(100, 10)
	for i := 0; i < 500; i++ {
		if b.MayContain(key("x", i)) {
			t.Fatalf("an empty filter said true for key %d", i)
		}
	}
	if NewBloom(0, 10).MayContain([]byte("a")) {
		t.Errorf("NewBloom(0, 10) says true for a key that was never added")
	}
}

func TestSizing_follows_the_budget(t *testing.T) {
	data := NewBloom(10000, 10).Marshal()
	if len(data) < 5 {
		t.Fatalf("Marshal returned %d bytes", len(data))
	}
	if data[0] != 7 {
		t.Errorf("k = %d for 10 bits per key; want 7", data[0])
	}
	if m := binary.BigEndian.Uint32(data[1:5]); m != 100032 {
		t.Errorf("m = %d; want 100032 (100000 rounded up to a multiple of 64)", m)
	}
	if len(data) != 5+8*(100032/64) {
		t.Errorf("serialised size = %d; want %d", len(data), 5+8*(100032/64))
	}
	small := NewBloom(0, 10).Marshal()
	if binary.BigEndian.Uint32(small[1:5]) != 64 {
		t.Errorf("the minimum size should be 64 bits, got m = %d", binary.BigEndian.Uint32(small[1:5]))
	}
	if k := NewBloom(10, 1).Marshal()[0]; k != 1 {
		t.Errorf("k = %d for 1 bit per key; want it clamped to 1", k)
	}
	if k := NewBloom(10, 100).Marshal()[0]; k != 30 {
		t.Errorf("k = %d for 100 bits per key; want it clamped to 30", k)
	}
}

func TestMarshal_round_trip(t *testing.T) {
	b := NewBloom(500, 12)
	for i := 0; i < 500; i++ {
		b.Add(key("k", i))
	}
	data := b.Marshal()
	c, err := UnmarshalBloom(data)
	if err != nil {
		t.Fatalf("UnmarshalBloom: %v", err)
	}
	for i := 0; i < 500; i++ {
		if !c.MayContain(key("k", i)) {
			t.Fatalf("restored filter lost key %d", i)
		}
	}
	if !bytes.Equal(c.Marshal(), data) {
		t.Errorf("marshalling the restored filter gives different bytes")
	}
	for i := 0; i < 300; i++ {
		if b.MayContain(key("other", i)) != c.MayContain(key("other", i)) {
			t.Fatalf("original and restored filters disagree on key other-%d", i)
		}
	}
}

func TestUnmarshal_rejects_bad_data(t *testing.T) {
	good := NewBloom(100, 10).Marshal()
	cases := map[string][]byte{
		"empty":       nil,
		"too short":   good[:3],
		"cut body":    good[:len(good)-1],
		"extra bytes": append(append([]byte{}, good...), 0),
		"k is zero":   append([]byte{0}, good[1:]...),
		"k too big":   append([]byte{31}, good[1:]...),
		"m is zero":   append([]byte{7, 0, 0, 0, 0}, good[5:]...),
	}
	for name, data := range cases {
		func() {
			defer func() {
				if r := recover(); r != nil {
					t.Errorf("%s: panicked: %v", name, r)
				}
			}()
			if _, err := UnmarshalBloom(data); err == nil {
				t.Errorf("%s: no error", name)
			}
		}()
	}
}
