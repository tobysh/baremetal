package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"hash/crc32"
	"reflect"
	"testing"
)

type rop struct {
	del      bool
	key, val string
}

func (db *DB) state(keys []string) map[string]string {
	m := map[string]string{}
	for _, k := range keys {
		if v, ok := db.Get(k); ok {
			m[k] = v
		}
	}
	return m
}

func applyModel(m map[string]string, o rop) {
	if o.del {
		delete(m, o.key)
	} else {
		m[o.key] = o.val
	}
}

var reviewOps = []rop{
	{false, "a", "1"}, {false, "b", "22"}, {false, "a", "333"}, {true, "b", ""},
	{false, "c", ""}, {true, "zzz", ""}, {false, "d", "last value"},
}
var reviewKeys = []string{"a", "b", "c", "d", "zzz"}

func buildDB(ops []rop) *DB {
	db := Open(nil)
	for _, o := range ops {
		if o.del {
			db.Delete(o.key)
		} else {
			db.Put(o.key, o.val)
		}
	}
	return db
}

func TestPut_Get_Delete(t *testing.T) {
	db := buildDB(reviewOps)
	want := map[string]string{}
	for _, o := range reviewOps {
		applyModel(want, o)
	}
	if got := db.state(reviewKeys); !reflect.DeepEqual(got, want) {
		t.Errorf("state = %v; want %v", got, want)
	}
	if v, ok := db.Get("c"); !ok || v != "" {
		t.Errorf("Get(c) = %q, %v; want an empty value that is present", v, ok)
	}
}

func TestLog_frame_layout(t *testing.T) {
	db := Open(nil)
	db.Put("k", "vv")
	log := db.Log()
	payload := []byte{1, 0, 0, 0, 1, 'k', 'v', 'v'}
	if len(log) != 8+len(payload) {
		t.Fatalf("log is %d bytes for one Put(\"k\", \"vv\"); want %d", len(log), 8+len(payload))
	}
	if !bytes.Equal(log[8:], payload) {
		t.Errorf("payload = % x; want % x", log[8:], payload)
	}
	if binary.BigEndian.Uint32(log[4:8]) != uint32(len(payload)) {
		t.Errorf("length field = %d; want %d", binary.BigEndian.Uint32(log[4:8]), len(payload))
	}
	want := crc32.ChecksumIEEE(log[4:])
	if got := binary.BigEndian.Uint32(log[0:4]); got != want {
		t.Errorf("crc = %08x; want %08x", got, want)
	}
}

func TestLog_returns_a_copy(t *testing.T) {
	db := Open(nil)
	db.Put("k", "v")
	l := db.Log()
	l[len(l)-1] ^= 0xFF
	if again := Open(db.Log()); again.state([]string{"k"})["k"] != "v" {
		t.Errorf("changing the slice returned by Log changed the database")
	}
}

func TestOpen_replays_the_log(t *testing.T) {
	db := buildDB(reviewOps)
	re := Open(db.Log())
	if !reflect.DeepEqual(re.state(reviewKeys), db.state(reviewKeys)) {
		t.Errorf("reopened state %v differs from %v", re.state(reviewKeys), db.state(reviewKeys))
	}
	if !bytes.Equal(re.Log(), db.Log()) {
		t.Errorf("reopening changed the log")
	}
}

func TestOpen_torn_tail_at_every_byte(t *testing.T) {
	var log []byte
	var ends []int
	for _, o := range reviewOps {
		db := Open(log)
		if o.del {
			db.Delete(o.key)
		} else {
			db.Put(o.key, o.val)
		}
		log = db.Log()
		ends = append(ends, len(log))
	}
	for cut := 0; cut <= len(log); cut++ {
		complete, good := 0, 0
		for i, e := range ends {
			if e <= cut {
				complete, good = i+1, e
			}
		}
		want := map[string]string{}
		for _, o := range reviewOps[:complete] {
			applyModel(want, o)
		}
		db := Open(log[:cut])
		if got := db.state(reviewKeys); !reflect.DeepEqual(got, want) {
			t.Fatalf("log cut at %d: state = %v; want %v", cut, got, want)
		}
		if len(db.Log()) != good {
			t.Fatalf("log cut at %d: kept %d bytes; want %d", cut, len(db.Log()), good)
		}
	}
}

func TestOpen_checksum_failure_discards_the_rest(t *testing.T) {
	var log []byte
	var ends []int
	for i := 0; i < 5; i++ {
		db := Open(log)
		db.Put(fmt.Sprintf("k%d", i), fmt.Sprintf("v%d", i))
		log = db.Log()
		ends = append(ends, len(log))
	}
	bad := append([]byte{}, log...)
	bad[ends[1]+9] ^= 0x04 // inside the third frame's payload
	db := Open(bad)
	if _, ok := db.Get("k1"); !ok {
		t.Errorf("k1, written before the damage, was lost")
	}
	for _, k := range []string{"k2", "k3", "k4"} {
		if _, ok := db.Get(k); ok {
			t.Errorf("%s from the damaged frame or after it was applied", k)
		}
	}
	db.Put("fresh", "ok")
	again := Open(db.Log())
	if v, _ := again.Get("fresh"); v != "ok" {
		t.Errorf("a write made after recovery was lost on the next Open; the damaged tail must be discarded")
	}
}

func TestOpen_copies_its_argument(t *testing.T) {
	db := buildDB(reviewOps[:3])
	log := db.Log()
	re := Open(log)
	for i := range log {
		log[i] = 0
	}
	if v, _ := re.Get("a"); v != "333" || !bytes.Equal(re.Log(), db.Log()) {
		t.Errorf("the DB kept a reference to the caller's slice")
	}
}

func TestCompact_shrinks_the_log_and_keeps_state(t *testing.T) {
	db := Open(nil)
	for i := 0; i < 300; i++ {
		db.Put(fmt.Sprintf("key%d", i%6), fmt.Sprintf("value-number-%d", i))
	}
	db.Delete("key2")
	before := len(db.Log())
	state := db.state([]string{"key0", "key1", "key2", "key3", "key4", "key5"})
	db.Compact()
	if len(db.Log()) >= before/20 {
		t.Errorf("log is %d bytes after Compact, from %d", len(db.Log()), before)
	}
	if got := db.state([]string{"key0", "key1", "key2", "key3", "key4", "key5"}); !reflect.DeepEqual(got, state) {
		t.Errorf("Compact changed the state: %v", got)
	}
	re := Open(db.Log())
	if got := re.state([]string{"key0", "key1", "key2", "key3", "key4", "key5"}); !reflect.DeepEqual(got, state) {
		t.Errorf("the compacted log replays to %v; want %v", got, state)
	}
	if _, ok := re.Get("key2"); ok {
		t.Errorf("a deleted key came back after compaction")
	}
}

func TestCompact_output_is_sorted_puts(t *testing.T) {
	db := Open(nil)
	db.Put("b", "2")
	db.Put("a", "1")
	db.Put("c", "3")
	db.Delete("c")
	db.Compact()
	want := Open(nil)
	want.Put("a", "1")
	want.Put("b", "2")
	if !bytes.Equal(db.Log(), want.Log()) {
		t.Errorf("compacted log = % x; want the frames for put a, put b: % x", db.Log(), want.Log())
	}
}
