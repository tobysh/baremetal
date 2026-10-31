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

func CheckEngine(open Opener, puts []KV) (out []string) {
	clean := NewFaultDisk(nil)
	func() {
		defer func() {
			if r := recover(); r != nil {
				out = append(out, fmt.Sprintf("fault-free run panicked: %v", r))
			}
		}()
		e := open(clean)
		for _, p := range puts {
			if err := e.Put(p.Key, p.Value); err != nil {
				out = append(out, "fault-free put failed: "+err.Error())
			}
		}
	}()
	if len(out) > 0 {
		return out
	}
	names := map[FaultKind]string{Short: "Short", Lost: "Lost", Crash: "Crash"}
	keepNames := []string{"zero", "one", "half", "all-but-one", "all"}
	for at := 0; at < clean.Ops(); at++ {
		for _, kind := range []FaultKind{Short, Lost, Crash} {
			keeps := []Keep{KeepZero}
			switch kind {
			case Short:
				keeps = []Keep{KeepZero, KeepOne, KeepHalf, KeepAllButOne}
			case Crash:
				keeps = []Keep{KeepZero, KeepOne, KeepHalf, KeepAllButOne, KeepAll}
			}
			for _, k := range keeps {
				label := fmt.Sprintf("%s at append %d keep=%s", names[kind], at, keepNames[k])
				for _, v := range runOne(open, puts, Fault{at, kind, k}) {
					out = append(out, label+": "+v)
				}
			}
		}
	}
	return out
}

func runOne(open Opener, puts []KV, f Fault) (out []string) {
	defer func() {
		if r := recover(); r != nil {
			out = append(out, fmt.Sprintf("panic: %v", r))
		}
	}()
	d := NewFaultDisk(&f)
	e := open(d)
	acked := map[string]string{}
	var inflight *KV
	for i := range puts {
		if err := e.Put(puts[i].Key, puts[i].Value); err != nil {
			inflight = &puts[i]
			break
		}
		acked[puts[i].Key] = puts[i].Value
	}
	d.Restart()
	e2 := open(d)
	seen := map[string]string{}
	present := map[string]bool{}
	for _, p := range puts {
		if _, done := present[p.Key]; done {
			continue
		}
		got, ok := e2.Get(p.Key)
		present[p.Key] = ok
		seen[p.Key] = got
		want, wasAcked := acked[p.Key]
		isInflight := inflight != nil && inflight.Key == p.Key
		allowed := (wasAcked && ok && got == want) || (isInflight && ok && got == inflight.Value) || (!wasAcked && !ok)
		if !allowed {
			switch {
			case wasAcked && !ok:
				out = append(out, fmt.Sprintf("acknowledged key %q lost", p.Key))
			case wasAcked:
				out = append(out, fmt.Sprintf("key %q is %q, want acknowledged %q", p.Key, got, want))
			default:
				out = append(out, fmt.Sprintf("key %q has invented value %q", p.Key, got))
			}
		}
	}
	if err := e2.Put("post-recovery", "ok"); err != nil {
		return append(out, "put after recovery failed: "+err.Error())
	}
	d.Restart()
	e3 := open(d)
	for k, v := range seen {
		got, ok := e3.Get(k)
		if ok != present[k] || got != v {
			out = append(out, fmt.Sprintf("key %q changed across a second restart", k))
		}
	}
	if got, ok := e3.Get("post-recovery"); !ok || got != "ok" {
		out = append(out, "write made after recovery was lost on the next restart")
	}
	return out
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
