package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"reflect"
	"testing"
)

type op struct {
	del      bool
	key, val string
}

func buildLog(ops []op) ([]byte, []int) {
	var log []byte
	ends := []int{}
	for _, o := range ops {
		if o.del {
			log = append(log, EncodeDelete(o.key)...)
		} else {
			log = append(log, EncodePut(o.key, o.val)...)
		}
		ends = append(ends, len(log))
	}
	return log, ends
}

func model(ops []op) map[string]string {
	m := map[string]string{}
	for _, o := range ops {
		if o.del {
			delete(m, o.key)
		} else {
			m[o.key] = o.val
		}
	}
	return m
}

var sampleOps = []op{
	{false, "alpha", "1"}, {false, "beta", "two"}, {false, "alpha", "three"},
	{true, "beta", ""}, {false, "gamma", ""}, {false, "", "empty-key"},
	{false, "delta", "a longer value so the record spans many bytes"}, {true, "nothing", ""},
}

func TestReplay_applies_puts_overwrites_and_deletes(t *testing.T) {
	log, _ := buildLog(sampleOps)
	state, valid := Replay(log)
	want := map[string]string{"alpha": "three", "gamma": "", "": "empty-key", "delta": "a longer value so the record spans many bytes"}
	if !reflect.DeepEqual(state, want) {
		t.Errorf("state = %v; want %v", state, want)
	}
	if valid != len(log) {
		t.Errorf("valid = %d; want the whole log (%d bytes)", valid, len(log))
	}
}

func TestReplay_empty_log(t *testing.T) {
	state, valid := Replay(nil)
	if state == nil || len(state) != 0 || valid != 0 {
		t.Errorf("Replay(nil) = %#v, %d; want an empty non-nil map and 0", state, valid)
	}
}

func TestReplay_survives_a_cut_at_every_byte(t *testing.T) {
	log, ends := buildLog(sampleOps)
	for cut := 0; cut <= len(log); cut++ {
		complete, goodBytes := 0, 0
		for i, e := range ends {
			if e <= cut {
				complete, goodBytes = i+1, e
			}
		}
		state, valid := Replay(log[:cut])
		if valid != goodBytes {
			t.Errorf("log cut at %d: valid = %d; want %d", cut, valid, goodBytes)
			continue
		}
		if want := model(sampleOps[:complete]); !reflect.DeepEqual(state, want) {
			t.Errorf("log cut at %d: state = %v; want %v (first %d records)", cut, state, want, complete)
		}
	}
}

func TestReplay_stops_at_garbage(t *testing.T) {
	log, _ := buildLog(sampleOps[:3])
	good := len(log)
	damaged := append(append([]byte{}, log...), 0x7F, 0, 0, 0, 1, 0, 0, 0, 1, 'k', 'v')
	state, valid := Replay(damaged)
	if valid != good {
		t.Errorf("unknown op byte: valid = %d; want %d", valid, good)
	}
	if want := model(sampleOps[:3]); !reflect.DeepEqual(state, want) {
		t.Errorf("state = %v; want %v", state, want)
	}
}

func TestReplay_huge_lengths_do_not_panic(t *testing.T) {
	log, _ := buildLog(sampleOps[:2])
	good := len(log)
	bad := append(append([]byte{}, log...), opPut)
	bad = binary.BigEndian.AppendUint32(bad, 0xFFFFFFFF)
	bad = binary.BigEndian.AppendUint32(bad, 0xFFFFFFFF)
	bad = append(bad, "xx"...)
	_, valid := Replay(bad)
	if valid != good {
		t.Errorf("record claiming 8 GB: valid = %d; want %d", valid, good)
	}
}

func TestRecover_truncates_the_torn_tail(t *testing.T) {
	log, ends := buildLog(sampleOps)
	cut := ends[4] + 6
	var buf bytes.Buffer
	buf.Write(log[:cut])
	state := Recover(&buf)
	if buf.Len() != ends[4] {
		t.Errorf("buffer length after Recover = %d; want %d", buf.Len(), ends[4])
	}
	if want := model(sampleOps[:5]); !reflect.DeepEqual(state, want) {
		t.Errorf("state = %v; want %v", state, want)
	}
}

func TestRecover_then_append_is_readable(t *testing.T) {
	log, ends := buildLog(sampleOps)
	var buf bytes.Buffer
	buf.Write(log[:ends[2]-3])
	Recover(&buf)
	buf.Write(EncodePut("fresh", "start"))
	state, valid := Replay(buf.Bytes())
	if valid != buf.Len() {
		t.Errorf("after recover and append, valid = %d of %d bytes: the new record is hidden behind the garbage", valid, buf.Len())
	}
	if state["fresh"] != "start" {
		t.Errorf("record written after recovery was lost: %v", state)
	}
}

func TestReplay_many_records(t *testing.T) {
	var ops []op
	for i := 0; i < 500; i++ {
		k := fmt.Sprintf("k%d", i%37)
		if i%5 == 4 {
			ops = append(ops, op{true, k, ""})
		} else {
			ops = append(ops, op{false, k, fmt.Sprint(i)})
		}
	}
	log, _ := buildLog(ops)
	state, valid := Replay(log)
	if valid != len(log) || !reflect.DeepEqual(state, model(ops)) {
		t.Errorf("replaying 500 records gave a different state than applying them directly")
	}
}
