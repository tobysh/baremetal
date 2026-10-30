package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"hash/crc32"
	"reflect"
	"testing"
)

// Independent fixtures prevent paired learner helpers from concealing a format bug.
func pipelineCapsule(code []byte, entry uint16) []byte {
	b := make([]byte, 12+len(code))
	copy(b, "BML1")
	binary.LittleEndian.PutUint16(b[4:6], entry)
	binary.LittleEndian.PutUint16(b[6:8], uint16(len(code)))
	binary.LittleEndian.PutUint32(b[8:12], crc32.ChecksumIEEE(code))
	copy(b[12:], code)
	return b
}
func pipelinePackage(plain []byte, key byte) []byte {
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
func TestPipelineBehavior(t *testing.T) {
	for i := 0; i < 24; i++ {
		key := fmt.Sprint("record-", i)
		value := fmt.Sprintf("data-%d\x00", i)
		code := []byte{1, 4, 's', 'k', 'i', 'p', 2, byte(len(key))}
		code = append(code, []byte(key)...)
		code = append(code, 3, byte(i+1), 0, 0)
		capsule := pipelineCapsule(code, 6)
		bundle := pipelinePackage(capsule, byte(i+1))
		before := append([]byte{}, bundle...)
		caps := map[string]string{key: value}
		got, err := ExecuteBundle(bundle, caps, 3, uint32(i+1))
		want := Trace{Outputs: []string{value}, Reads: []string{key}, Ticks: uint32(i + 1)}
		if err != nil || !reflect.DeepEqual(got, want) || !bytes.Equal(bundle, before) || caps[key] != value || len(caps) != 1 {
			t.Fatal(got, err)
		}
	}
}
func TestPipelineValidatesEveryLayer(t *testing.T) {
	valid := pipelineCapsule([]byte{1, 1, 'x', 0}, 0)
	badCapsule := append([]byte{}, valid...)
	badCapsule[8] ^= 1
	bad := [][]byte{nil, pipelinePackage([]byte("not a capsule"), 2), pipelinePackage(badCapsule, 3), pipelinePackage(pipelineCapsule([]byte{99, 0}, 0), 4), pipelinePackage(pipelineCapsule([]byte{1, 2, 'h', 'i', 0}, 2), 5)}
	outer := pipelinePackage(valid, 6)
	outer[11] ^= 1
	bad = append(bad, outer)
	for i, b := range bad {
		got, err := ExecuteBundle(b, nil, 64, 100)
		if err == nil || !reflect.DeepEqual(got, Trace{}) {
			t.Fatal("invalid layer accepted", i)
		}
	}
}
func TestPipelineCapabilitiesAndBudgets(t *testing.T) {
	code := []byte{1, 1, 'x', 2, 1, 'k', 3, 9, 0, 0}
	bundle := pipelinePackage(pipelineCapsule(code, 0), 19)
	for _, c := range []struct {
		caps  map[string]string
		ops   int
		ticks uint32
	}{{nil, 4, 9}, {map[string]string{"k": "v"}, 3, 9}, {map[string]string{"k": "v"}, 4, 8}} {
		got, err := ExecuteBundle(bundle, c.caps, c.ops, c.ticks)
		if err == nil || !reflect.DeepEqual(got, Trace{}) {
			t.Fatal("capability or budget failure leaked partial output", got)
		}
	}
}
func TestPipelineSkipsUnexecutedCapabilityRead(t *testing.T) {
	code := []byte{2, 6, 'd', 'e', 'n', 'i', 'e', 'd', 1, 2, 'o', 'k', 0}
	got, err := ExecuteBundle(pipelinePackage(pipelineCapsule(code, 8), 19), nil, 2, 0)
	if err != nil || !reflect.DeepEqual(got, Trace{Outputs: []string{"ok"}, Reads: []string{}}) {
		t.Fatal(got, err)
	}
}
