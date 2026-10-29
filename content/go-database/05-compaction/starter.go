package main

import (
	"encoding/binary"
	"errors"
	"fmt"
)

type Entry struct {
	Key    string
	Value  []byte
	Delete bool
}

// EncodeEntries writes each entry as: flag u8, keyLen u32, valLen u32, key, value.
func EncodeEntries(entries []Entry) []byte {
	var out []byte
	for _, e := range entries {
		var flag byte
		val := e.Value
		if e.Delete {
			flag, val = 1, nil
		}
		out = append(out, flag)
		out = binary.BigEndian.AppendUint32(out, uint32(len(e.Key)))
		out = binary.BigEndian.AppendUint32(out, uint32(len(val)))
		out = append(out, e.Key...)
		out = append(out, val...)
	}
	return out
}

func DecodeEntries(data []byte) ([]Entry, error) {
	var out []Entry
	for len(data) > 0 {
		if len(data) < 9 {
			return nil, errors.New("truncated entry header")
		}
		kl := uint64(binary.BigEndian.Uint32(data[1:5]))
		vl := uint64(binary.BigEndian.Uint32(data[5:9]))
		if uint64(len(data)-9) < kl+vl {
			return nil, errors.New("truncated entry body")
		}
		e := Entry{Key: string(data[9 : 9+kl]), Delete: data[0] == 1}
		e.Value = append([]byte(nil), data[9+kl:9+kl+vl]...)
		out = append(out, e)
		data = data[9+kl+vl:]
	}
	return out, nil
}

func Compact(entries []Entry, dropTombstones bool) []Entry {
	return nil
}

func CompactLog(log []byte) ([]byte, error) {
	return log, nil
}

// Try it with Run.
func main() {
	log := EncodeEntries([]Entry{
		{Key: "a", Value: []byte("1")},
		{Key: "b", Value: []byte("2")},
		{Key: "a", Value: []byte("3")},
		{Key: "b", Delete: true},
	})
	small, _ := CompactLog(log)
	fmt.Println(len(log), "->", len(small))
}
