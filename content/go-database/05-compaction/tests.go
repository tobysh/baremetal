package main

import (
	"bytes"
	"fmt"
	"reflect"
	"testing"
)

func put(k, v string) Entry { return Entry{Key: k, Value: []byte(v)} }
func del(k string) Entry    { return Entry{Key: k, Delete: true} }

func sameEntries(a, b []Entry) bool {
	if len(a) != len(b) {
		return false
	}
	for i := range a {
		if a[i].Key != b[i].Key || a[i].Delete != b[i].Delete || !bytes.Equal(a[i].Value, b[i].Value) {
			return false
		}
	}
	return true
}

func TestCompact_keeps_the_newest_and_sorts(t *testing.T) {
	in := []Entry{put("c", "1"), put("a", "1"), put("b", "1"), put("a", "2"), put("c", "3"), put("a", "4")}
	got := Compact(in, true)
	want := []Entry{put("a", "4"), put("b", "1"), put("c", "3")}
	if !sameEntries(got, want) {
		t.Errorf("Compact = %v; want %v", got, want)
	}
}

func TestCompact_drops_tombstones_when_asked(t *testing.T) {
	in := []Entry{put("a", "1"), put("b", "2"), del("a"), put("c", "3"), del("c"), put("c", "4"), del("zzz")}
	got := Compact(in, true)
	want := []Entry{put("b", "2"), put("c", "4")}
	if !sameEntries(got, want) {
		t.Errorf("Compact(.., true) = %v; want %v", got, want)
	}
}

func TestCompact_keeps_tombstones_otherwise(t *testing.T) {
	in := []Entry{put("a", "1"), put("b", "2"), del("a"), put("c", "3"), del("c"), put("c", "4"), del("zzz")}
	got := Compact(in, false)
	want := []Entry{del("a"), put("b", "2"), put("c", "4"), del("zzz")}
	if !sameEntries(got, want) {
		t.Errorf("Compact(.., false) = %v; want %v", got, want)
	}
}

func TestCompact_empty_input(t *testing.T) {
	for _, drop := range []bool{true, false} {
		got := Compact(nil, drop)
		if got == nil || len(got) != 0 {
			t.Errorf("Compact(nil, %v) = %#v; want an empty, non-nil slice", drop, got)
		}
	}
}

func TestCompact_does_not_modify_its_input(t *testing.T) {
	in := []Entry{put("b", "1"), put("a", "2"), put("b", "3"), del("a")}
	snapshot := []Entry{put("b", "1"), put("a", "2"), put("b", "3"), del("a")}
	out := Compact(in, false)
	if len(out) > 0 && len(out[0].Value) > 0 {
		out[0].Value[0] = '!'
	}
	if !reflect.DeepEqual(in, snapshot) {
		t.Errorf("the input slice or its values were changed: %v", in)
	}
}

func TestCompact_is_idempotent(t *testing.T) {
	var in []Entry
	for i := 0; i < 300; i++ {
		k := fmt.Sprintf("key%02d", (i*7)%23)
		if i%6 == 5 {
			in = append(in, del(k))
		} else {
			in = append(in, put(k, fmt.Sprint(i)))
		}
	}
	once := Compact(in, true)
	twice := Compact(once, true)
	if !sameEntries(once, twice) {
		t.Errorf("compacting twice changed the result")
	}
	if len(once) > 23 {
		t.Errorf("%d entries remain for 23 distinct keys", len(once))
	}
}

func TestCompactLog_shrinks_and_preserves_state(t *testing.T) {
	var in []Entry
	for i := 0; i < 200; i++ {
		in = append(in, put(fmt.Sprintf("k%d", i%10), fmt.Sprintf("version-%d", i)))
	}
	in = append(in, del("k3"), del("k4"))
	log := EncodeEntries(in)
	small, err := CompactLog(log)
	if err != nil {
		t.Fatalf("CompactLog: %v", err)
	}
	if len(small) >= len(log)/5 {
		t.Errorf("compacted log is %d bytes from %d; expected a big reduction", len(small), len(log))
	}
	got, err := DecodeEntries(small)
	if err != nil {
		t.Fatalf("compacted log does not decode: %v", err)
	}
	want := Compact(in, true)
	if !sameEntries(got, want) || len(got) != 8 {
		t.Errorf("compacted log holds %d entries %v; want the 8 live keys %v", len(got), got, want)
	}
}

func TestCompactLog_reports_damaged_input(t *testing.T) {
	log := EncodeEntries([]Entry{put("a", "1"), put("b", "2")})
	if _, err := CompactLog(log[:len(log)-1]); err == nil {
		t.Errorf("a truncated log was compacted without an error")
	}
	out, err := CompactLog(nil)
	if err != nil || len(out) != 0 {
		t.Errorf("CompactLog(nil) = %v, %v; want empty and no error", out, err)
	}
}
