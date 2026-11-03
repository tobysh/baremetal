package main

import "fmt"
import (
	"encoding/binary"
	"errors"
)

type DirEntry struct {
	Name  string
	Inode uint32
}

var ErrDir = errors.New("bad directory")

func validName(s string) bool {
	if len(s) < 1 || len(s) > 12 {
		return false
	}
	for _, b := range []byte(s) {
		if b == 0 || b == '/' || b > 127 {
			return false
		}
	}
	return true
}

func EncodeDir(entries []DirEntry) ([]byte, error) { return nil, nil }
func DecodeDir(data []byte) ([]DirEntry, error)    { return nil, nil }
func main() {
	b, e := EncodeDir([]DirEntry{{"notes", 3}})
	fmt.Printf("%x %v\n", b, e)
	fmt.Println(DecodeDir(b))
}

// Supplied endian helpers for the record inode field.
func readInodeNumber(b []byte) uint32   { return binary.BigEndian.Uint32(b) }
func putInodeNumber(b []byte, n uint32) { binary.BigEndian.PutUint32(b, n) }
