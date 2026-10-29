package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
)

const (
	opPut    byte = 1
	opDelete byte = 2
)

func encode(op byte, key, value string) []byte {
	rec := make([]byte, 9+len(key)+len(value))
	rec[0] = op
	binary.BigEndian.PutUint32(rec[1:5], uint32(len(key)))
	binary.BigEndian.PutUint32(rec[5:9], uint32(len(value)))
	copy(rec[9:], key)
	copy(rec[9+len(key):], value)
	return rec
}

func EncodePut(key, value string) []byte { return encode(opPut, key, value) }
func EncodeDelete(key string) []byte     { return encode(opDelete, key, "") }

func Replay(log []byte) (map[string]string, int) {
	return nil, 0
}

func Recover(buf *bytes.Buffer) map[string]string {
	return nil
}

// Try it with Run.
func main() {
	var log bytes.Buffer
	log.Write(EncodePut("a", "1"))
	log.Write(EncodePut("b", "2"))
	log.Write(EncodeDelete("a"))
	log.Write(EncodePut("c", "3")[:5]) // torn write
	fmt.Println(Recover(&log), log.Len())
}
