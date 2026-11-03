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

func EncodeDir(entries []DirEntry) ([]byte, error) {
	out := make([]byte, len(entries)*16)
	seen := map[string]bool{}
	for i, e := range entries {
		if !validName(e.Name) || e.Inode == 0 || seen[e.Name] {
			return nil, ErrDir
		}
		seen[e.Name] = true
		copy(out[i*16:], e.Name)
		binary.BigEndian.PutUint32(out[i*16+12:], e.Inode)
	}
	return out, nil
}
func DecodeDir(data []byte) ([]DirEntry, error) {
	if len(data)%16 != 0 {
		return nil, ErrDir
	}
	out := []DirEntry{}
	seen := map[string]bool{}
	for i := 0; i < len(data); i += 16 {
		name := data[i : i+12]
		end := 12
		for j, b := range name {
			if b == 0 {
				end = j
				break
			}
		}
		for _, b := range name[end:] {
			if b != 0 {
				return nil, ErrDir
			}
		}
		s := string(name[:end])
		ino := binary.BigEndian.Uint32(data[i+12:])
		if !validName(s) || ino == 0 || seen[s] {
			return nil, ErrDir
		}
		seen[s] = true
		out = append(out, DirEntry{s, ino})
	}
	return out, nil
}
func main() {
	b, e := EncodeDir([]DirEntry{{"notes", 3}})
	fmt.Printf("%x %v\n", b, e)
	fmt.Println(DecodeDir(b))
}

// Supplied endian helpers for the record inode field.
func readInodeNumber(b []byte) uint32   { return binary.BigEndian.Uint32(b) }
func putInodeNumber(b []byte, n uint32) { binary.BigEndian.PutUint32(b, n) }
