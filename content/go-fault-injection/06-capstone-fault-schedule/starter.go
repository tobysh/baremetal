package main

import (
	"encoding/binary"
	"errors"
	"fmt"
	"hash/crc32"
)

var (
	ErrIO    = errors.New("i/o error")
	ErrCrash = errors.New("crash")
)

type FaultKind int

const (
	Short FaultKind = iota + 1
	Lost
	Crash
)

type Keep int

const (
	KeepZero Keep = iota
	KeepOne
	KeepHalf
	KeepAllButOne
	KeepAll
)

// Fault hits the At-th (0-based) Append call of a FaultDisk.
type Fault struct {
	At   int
	Kind FaultKind
	Keep Keep
}

type FaultDisk struct {
	data  []byte
	ops   int
	fault *Fault
	dead  bool
}

func NewFaultDisk(f *Fault) *FaultDisk { return &FaultDisk{fault: f} }

func keepCount(k Keep, n int) int {
	switch k {
	case KeepOne:
		if n < 1 {
			return n
		}
		return 1
	case KeepHalf:
		return n / 2
	case KeepAllButOne:
		if n < 1 {
			return 0
		}
		return n - 1
	case KeepAll:
		return n
	}
	return 0
}

func (d *FaultDisk) Append(p []byte) (int, error) {
	idx := d.ops
	d.ops++
	if d.dead {
		return 0, ErrCrash
	}
	if d.fault != nil && d.fault.At == idx {
		n := keepCount(d.fault.Keep, len(p))
		switch d.fault.Kind {
		case Lost:
			return 0, ErrIO
		case Short:
			if n >= len(p) && len(p) > 0 {
				n = len(p) - 1
			}
			d.data = append(d.data, p[:n]...)
			return n, ErrIO
		case Crash:
			d.data = append(d.data, p[:n]...)
			d.dead = true
			return n, ErrCrash
		}
	}
	d.data = append(d.data, p...)
	return len(p), nil
}

func (d *FaultDisk) Bytes() []byte { return d.data }
func (d *FaultDisk) Ops() int      { return d.ops }

func (d *FaultDisk) Truncate(n int) error {
	if d.dead {
		return ErrCrash
	}
	if n < len(d.data) {
		d.data = d.data[:n]
	}
	return nil
}

// Restart models a reboot: the disk works again and no fault is scheduled.
func (d *FaultDisk) Restart() { d.dead = false; d.fault = nil }

type Engine interface {
	Put(key, value string) error
	Get(key string) (string, bool)
}

type Opener func(d *FaultDisk) Engine

type KV struct{ Key, Value string }

// ---- the engine under test: a checksummed append-only log ----

type LogStore struct {
	d    *FaultDisk
	m    map[string]string
	size int
}

func encodeRecord(key, value string) []byte {
	payload := append([]byte{byte(len(key))}, key...)
	payload = append(payload, value...)
	rec := make([]byte, 8, 8+len(payload))
	binary.BigEndian.PutUint32(rec, uint32(len(payload)))
	binary.BigEndian.PutUint32(rec[4:], crc32.ChecksumIEEE(payload))
	return append(rec, payload...)
}

func OpenLog(d *FaultDisk) Engine {
	s := &LogStore{d: d, m: map[string]string{}}
	b := d.Bytes()
	pos := 0
	for len(b)-pos >= 8 {
		n := int(binary.BigEndian.Uint32(b[pos:]))
		if n < 1 || n > len(b)-pos-8 {
			break
		}
		payload := b[pos+8 : pos+8+n]
		if crc32.ChecksumIEEE(payload) != binary.BigEndian.Uint32(b[pos+4:]) {
			break
		}
		kl := int(payload[0])
		if kl+1 > len(payload) {
			break
		}
		s.m[string(payload[1:1+kl])] = string(payload[1+kl:])
		pos += 8 + n
	}
	s.size = pos
	d.Truncate(pos) // drop the damaged tail so new records stay reachable
	return s
}

func (s *LogStore) Put(key, value string) error {
	rec := encodeRecord(key, value)
	if n, err := s.d.Append(rec); err != nil {
		if n > 0 {
			s.d.Truncate(s.size) // undo a short write
		}
		return err
	}
	s.size += len(rec)
	s.m[key] = value
	return nil
}

func (s *LogStore) Get(key string) (string, bool) { v, ok := s.m[key]; return v, ok }

// CheckEngine must return the violated invariants (nil when there are none).
// This placeholder reports nothing, so it never catches a broken engine.
func CheckEngine(open Opener, puts []KV) []string {
	return nil
}

func main() {
	d := NewFaultDisk(&Fault{At: 1, Kind: Crash, Keep: KeepHalf})
	e := OpenLog(d)
	fmt.Println("put a:", e.Put("a", "1"))
	fmt.Println("put b:", e.Put("b", "2"))
	d.Restart()
	e = OpenLog(d)
	_, hasB := e.Get("b")
	v, hasA := e.Get("a")
	fmt.Println("after restart: a =", v, hasA, "b present:", hasB)
	fmt.Println("violations on a correct engine:", CheckEngine(OpenLog, []KV{{"a", "1"}, {"b", "2"}}))
}
