package main

import (
	"bytes"
	"encoding/hex"
	"errors"
	"runtime"
	"testing"
)

type tRand struct{ s uint64 }

func (r *tRand) next() uint64 {
	r.s = r.s*6364136223846793005 + 1442695040888963407
	return r.s >> 33
}
func (r *tRand) intn(n int) int { return int(r.next() % uint64(n)) }

func unhex(s string) []byte {
	b, err := hex.DecodeString(s)
	if err != nil {
		panic(err)
	}
	return b
}

// skewed returns n bytes drawn from a skewed distribution over `alpha` symbols, with runs.
func skewed(r *tRand, n, alpha int) []byte {
	out := make([]byte, 0, n)
	for len(out) < n {
		s := byte(r.intn(alpha) * r.intn(alpha) / alpha * 7 % 256)
		run := 1
		if r.intn(4) == 0 {
			run = 1 + r.intn(200)
		}
		for i := 0; i < run && len(out) < n; i++ {
			out = append(out, s)
		}
	}
	return out
}

func allocated(f func()) uint64 {
	var a, b runtime.MemStats
	runtime.GC()
	runtime.ReadMemStats(&a)
	f()
	runtime.ReadMemStats(&b)
	return b.TotalAlloc - a.TotalAlloc
}

type bitItem struct {
	v uint64
	n int
}

func refPack(items []bitItem) []byte {
	s := ""
	for _, it := range items {
		for i := it.n - 1; i >= 0; i-- {
			s += string(rune('0' + (it.v>>uint(i))&1))
		}
	}
	for len(s)%8 != 0 {
		s += "0"
	}
	out := make([]byte, len(s)/8)
	for i := range out {
		for j := 0; j < 8; j++ {
			out[i] = out[i]<<1 | (s[i*8+j] - '0')
		}
	}
	return out
}

func writeAll(items []bitItem) *BitWriter {
	w := &BitWriter{}
	for _, it := range items {
		w.WriteBits(it.v, it.n)
	}
	return w
}

func TestWriter_known_answers(t *testing.T) {
	a := []bitItem{{5, 3}, {1, 1}, {255, 8}, {0, 2}}
	b := []bitItem{{0x1234, 13}, {3, 2}, {0xDEADBEEFCAFEBABE, 64}, {1, 1}}
	if got := hex.EncodeToString(refPack(a)); got != "bff0" {
		t.Fatalf("string reference packed known bits to %s; want bff0", got)
	}
	if got := hex.EncodeToString(writeAll(a).Bytes()); got != "bff0" {
		t.Errorf("101 1 11111111 00 packed to %s; want bff0", got)
	}
	if got := hex.EncodeToString(writeAll(b).Bytes()); got != "91a7bd5b7ddf95fd757d" {
		t.Errorf("mixed widths up to 64 bits packed to %s; want 91a7bd5b7ddf95fd757d", got)
	}
}

func TestWriter_length_and_masking(t *testing.T) {
	w := &BitWriter{}
	if w.Len() != 0 || len(w.Bytes()) != 0 {
		t.Fatalf("empty writer: Len=%d, %d bytes", w.Len(), len(w.Bytes()))
	}
	w.WriteBits(0xFFFF, 3) // only the low 3 bits count
	if w.Len() != 3 || !bytes.Equal(w.Bytes(), []byte{0xE0}) {
		t.Errorf("after WriteBits(0xFFFF,3): Len=%d Bytes=% x; want 3 and e0", w.Len(), w.Bytes())
	}
	w.WriteBits(0, 0)
	if w.Len() != 3 {
		t.Errorf("writing 0 bits changed Len to %d", w.Len())
	}
	snap := w.Bytes()
	snap[0] = 0
	w.WriteBits(1, 1)
	if !bytes.Equal(w.Bytes(), []byte{0xF0}) {
		t.Errorf("Bytes must return a copy and the writer must go on after Bytes: got % x", w.Bytes())
	}
}

func TestWriter_matches_string_reference(t *testing.T) {
	r := &tRand{s: 7}
	for round := 0; round < 300; round++ {
		var items []bitItem
		for i, n := 0, r.intn(40); i < n; i++ {
			w := r.intn(65)
			items = append(items, bitItem{r.next()<<31 ^ r.next(), w})
		}
		got := writeAll(items).Bytes()
		if want := refPack(items); !bytes.Equal(got, want) {
			t.Fatalf("round %d: items %v\n got % x\nwant % x", round, items, got, want)
		}
	}
}

func TestReader_round_trip_and_positions(t *testing.T) {
	r := &tRand{s: 99}
	for round := 0; round < 300; round++ {
		var items []bitItem
		total := 0
		for i, n := 0, 1+r.intn(30); i < n; i++ {
			w := r.intn(65)
			v := r.next()<<31 ^ r.next()
			if w < 64 {
				v &= 1<<uint(w) - 1
			}
			items = append(items, bitItem{v, w})
			total += w
		}
		br := NewBitReader(writeAll(items).Bytes())
		pos := 0
		for _, it := range items {
			got, err := br.ReadBits(it.n)
			if err != nil || got != it.v {
				t.Fatalf("round %d: ReadBits(%d) = %x, %v; want %x", round, it.n, got, err, it.v)
			}
			pos += it.n
			if br.Pos() != pos || br.Remaining() != (total+7)/8*8-pos {
				t.Fatalf("round %d: Pos=%d Remaining=%d after %d bits", round, br.Pos(), br.Remaining(), pos)
			}
		}
	}
}

func TestReader_short_read_consumes_nothing(t *testing.T) {
	br := NewBitReader([]byte{0xA5, 0x0F})
	if v, err := br.ReadBits(12); err != nil || v != 0xA50 {
		t.Fatalf("ReadBits(12) = %x, %v; want a50", v, err)
	}
	if _, err := br.ReadBits(5); !errors.Is(err, ErrShort) {
		t.Fatalf("5 bits with 4 left: err = %v; want ErrShort", err)
	}
	if br.Pos() != 12 || br.Remaining() != 4 {
		t.Errorf("a failed read moved the reader: Pos=%d Remaining=%d", br.Pos(), br.Remaining())
	}
	if v, err := br.ReadBits(4); err != nil || v != 0xF {
		t.Errorf("the last 4 bits = %x, %v; want f", v, err)
	}
	if v, err := br.ReadBits(0); err != nil || v != 0 {
		t.Errorf("ReadBits(0) at the end = %x, %v; want 0, nil", v, err)
	}
	if _, err := NewBitReader(nil).ReadBits(1); !errors.Is(err, ErrShort) {
		t.Errorf("reading from nothing: err = %v; want ErrShort", err)
	}
}
