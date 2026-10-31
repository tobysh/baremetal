package main

import (
	"encoding/binary"
	"errors"
	"fmt"
	"hash/crc32"
)

var ErrCorrupt = errors.New("corrupt file")

type Disk interface {
	WriteFile(name string, data []byte) error // not atomic: may leave a torn prefix after a crash
	Rename(oldName, newName string) error     // atomic
	Remove(name string) error
	ReadFile(name string) ([]byte, bool)
}

func encode(data []byte) []byte {
	b := make([]byte, 4, 8+len(data))
	binary.BigEndian.PutUint32(b, uint32(len(data)))
	b = append(b, data...)
	return binary.BigEndian.AppendUint32(b, crc32.ChecksumIEEE(data))
}

func decode(b []byte) ([]byte, bool) {
	if len(b) < 8 {
		return nil, false
	}
	n := binary.BigEndian.Uint32(b)
	if uint64(len(b)) != 8+uint64(n) {
		return nil, false
	}
	data := b[4 : 4+n]
	if crc32.ChecksumIEEE(data) != binary.BigEndian.Uint32(b[4+n:]) {
		return nil, false
	}
	return append([]byte{}, data...), true
}

// Save currently overwrites db in place.
func Save(d Disk, data []byte) error {
	return d.WriteFile("db", encode(data))
}

func Load(d Disk) ([]byte, bool, error) {
	raw, ok := d.ReadFile("db")
	if !ok {
		return nil, false, nil
	}
	data, ok := decode(raw)
	if !ok {
		return nil, false, ErrCorrupt
	}
	return data, true, nil
}

type memDisk map[string][]byte

func (m memDisk) WriteFile(n string, b []byte) error { m[n] = append([]byte{}, b...); return nil }
func (m memDisk) Rename(o, n string) error           { m[n] = m[o]; delete(m, o); return nil }
func (m memDisk) Remove(n string) error              { delete(m, n); return nil }
func (m memDisk) ReadFile(n string) ([]byte, bool)   { b, ok := m[n]; return b, ok }

func main() {
	d := memDisk{}
	Save(d, []byte("version one"))
	data, found, err := Load(d)
	fmt.Printf("%q %v %v\n", data, found, err)
}
