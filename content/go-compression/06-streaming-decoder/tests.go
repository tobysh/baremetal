package main

import (
	"bytes"
	"encoding/binary"
	"encoding/hex"
	"errors"
	"fmt"
	"runtime"
	"sort"
	"strings"
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

// refHuff is an independent, string-based decoder for the HuffEncode format.
// ok is false for anything the format forbids. ends[i] is the payload bit position where symbol i ends.
func refHuff(enc []byte) (out []byte, ends []int, hdr int, ok bool) {
	uv := func(b []byte) (uint64, int, bool) {
		v, n := binary.Uvarint(b)
		if n <= 0 || n != len(binary.AppendUvarint(nil, v)) {
			return 0, 0, false
		}
		return v, n, true
	}
	n, c1, ok1 := uv(enc)
	if !ok1 {
		return nil, nil, 0, false
	}
	k, c2, ok2 := uv(enc[c1:])
	if !ok2 || k > 256 || uint64(len(enc)-c1-c2) < 2*k {
		return nil, nil, 0, false
	}
	pos := c1 + c2
	type sl struct{ s, l int }
	var list []sl
	var kraft uint64
	prev := -1
	for i := uint64(0); i < k; i++ {
		s, l := int(enc[pos]), int(enc[pos+1])
		pos += 2
		if s <= prev || l < 1 || l > 48 {
			return nil, nil, 0, false
		}
		prev = s
		list = append(list, sl{s, l})
		kraft += 1 << uint(48-l)
	}
	if (k == 0 && n != 0) || (k == 1 && list[0].l != 1) || (k >= 2 && kraft != 1<<48) {
		return nil, nil, 0, false
	}
	sort.Slice(list, func(i, j int) bool {
		if list[i].l != list[j].l {
			return list[i].l < list[j].l
		}
		return list[i].s < list[j].s
	})
	table := map[string]byte{}
	code, plen := uint64(0), 0
	for _, e := range list {
		code <<= uint(e.l - plen)
		plen = e.l
		table[fmt.Sprintf("%0*b", e.l, code)] = byte(e.s)
		code++
	}
	var bitstr strings.Builder
	for _, b := range enc[pos:] {
		fmt.Fprintf(&bitstr, "%08b", b)
	}
	bits := bitstr.String()
	at := 0
	for uint64(len(out)) < n {
		found := false
		for l := 1; l <= 48 && at+l <= len(bits); l++ {
			if s, hit := table[bits[at:at+l]]; hit {
				out = append(out, s)
				at += l
				ends = append(ends, at)
				found = true
				break
			}
		}
		if !found {
			return nil, nil, 0, false
		}
	}
	if len(bits)-at >= 8 || strings.Trim(bits[at:], "0") != "" {
		return nil, nil, 0, false
	}
	return out, ends, pos, true
}

func huffInputs() [][]byte {
	r := &tRand{s: 77}
	ins := [][]byte{nil, []byte("a"), []byte("aaaaaaaa"), []byte("abracadabra"), []byte("ab")}
	for i := 0; i < 60; i++ {
		ins = append(ins, skewed(r, r.intn(400), 1+r.intn(40)))
	}
	all := make([]byte, 0, 600)
	for i := 0; i < 600; i++ {
		all = append(all, byte(i*7))
	}
	return append(ins, all)
}

// mutate returns a damaged copy of enc: bit flip, cut, appended byte or byte overwrite.
func mutate(r *tRand, enc []byte) []byte {
	m := append([]byte{}, enc...)
	switch r.intn(4) {
	case 0:
		if len(m) > 0 {
			m[r.intn(len(m))] ^= 1 << uint(r.intn(8))
		}
	case 1:
		m = m[:r.intn(len(m)+1)]
	case 2:
		m = append(m, byte(r.intn(256)))
	default:
		if len(m) > 0 {
			m[r.intn(len(m))] = byte(r.intn(256))
		}
	}
	return m
}

func feed(t *testing.T, enc []byte, sizes func() int) (out []byte, perByte []int, err error) {
	d := NewStreamDecoder()
	for pos := 0; pos < len(enc); {
		n := sizes()
		if pos+n > len(enc) {
			n = len(enc) - pos
		}
		got, werr := d.Write(enc[pos : pos+n])
		out = append(out, got...)
		pos += n
		perByte = append(perByte, len(out))
		if werr != nil {
			if _, again := d.Write([]byte{1}); again != werr {
				t.Fatalf("error must be sticky: first %v, then %v", werr, again)
			}
			return out, perByte, werr
		}
	}
	return out, perByte, d.Close()
}

func TestStream_matches_whole_buffer_for_every_chunking(t *testing.T) {
	r := &tRand{s: 8}
	for i, d := range huffInputs() {
		enc := HuffEncode(d)
		for name, sizes := range map[string]func() int{
			"whole":   func() int { return len(enc) + 1 },
			"1 byte":  func() int { return 1 },
			"random":  func() int { return r.intn(7) }, // includes empty writes
			"3 bytes": func() int { return 3 },
		} {
			out, _, err := feed(t, enc, sizes)
			if err != nil || !bytes.Equal(out, d) {
				t.Fatalf("input %d, chunking %q: err=%v, %d of %d bytes right", i, name, err, len(out), len(d))
			}
		}
	}
}

func TestStream_split_at_every_single_position(t *testing.T) {
	enc := HuffEncode([]byte("abracadabra abracadabra"))
	for cut := 0; cut <= len(enc); cut++ {
		d := NewStreamDecoder()
		a, e1 := d.Write(enc[:cut])
		b, e2 := d.Write(enc[cut:])
		if e1 != nil || e2 != nil || d.Close() != nil || string(a)+string(b) != "abracadabra abracadabra" {
			t.Fatalf("split at %d: %q %q %v %v", cut, a, b, e1, e2)
		}
	}
}

func TestStream_emits_each_symbol_as_soon_as_its_bits_arrive(t *testing.T) {
	for _, d := range [][]byte{[]byte("abracadabra abracadabra"), skewed(&tRand{s: 2}, 300, 12)} {
		enc := HuffEncode(d)
		ref, ends, hdr, ok := refHuff(enc)
		if !ok || !bytes.Equal(ref, d) {
			t.Fatal("reference failed")
		}
		_, perByte, err := feed(t, enc, func() int { return 1 })
		if err != nil {
			t.Fatal(err)
		}
		for j := range perByte {
			avail := 0
			if j+1 > hdr {
				avail = (j + 1 - hdr) * 8
			}
			want := 0
			for want < len(ends) && ends[want] <= avail {
				want++
			}
			if perByte[j] != want {
				t.Fatalf("after %d byte(s) the decoder has produced %d symbols; %d are fully available (do not wait for more input)", j+1, perByte[j], want)
			}
		}
	}
}

func TestStream_truncated_input_fails_at_close(t *testing.T) {
	enc := HuffEncode([]byte("abracadabra"))
	for cut := 0; cut < len(enc); cut++ {
		d := NewStreamDecoder()
		if _, err := d.Write(enc[:cut]); err != nil {
			t.Fatalf("a valid prefix of %d bytes made Write fail: %v", cut, err)
		}
		if err := d.Close(); !errors.Is(err, ErrTruncated) {
			t.Fatalf("closed after %d of %d bytes: err = %v; want ErrTruncated", cut, len(enc), err)
		}
	}
}

func TestStream_rejects_trailing_data_as_it_arrives(t *testing.T) {
	enc := HuffEncode([]byte("abracadabra"))
	d := NewStreamDecoder()
	if _, err := d.Write(enc); err != nil {
		t.Fatal(err)
	}
	if _, err := d.Write([]byte{0}); !errors.Is(err, ErrCorrupt) {
		t.Errorf("a byte after the end: err = %v; want ErrCorrupt", err)
	}
	if err := d.Close(); !errors.Is(err, ErrCorrupt) {
		t.Errorf("Close after the error returned %v; want the same ErrCorrupt", err)
	}
}

func TestStream_agrees_with_reference_on_damaged_input(t *testing.T) {
	r := &tRand{s: 4321}
	ins := huffInputs()
	for i := 0; i < 1500; i++ {
		enc := mutate(r, HuffEncode(ins[r.intn(len(ins))]))
		ref, _, _, ok := refHuff(enc)
		var out []byte
		var err error
		func() {
			defer func() {
				if p := recover(); p != nil {
					err = fmt.Errorf("panic: %v", p)
				}
			}()
			out, _, err = feed(t, enc, func() int { return 1 + r.intn(5) })
		}()
		if ok != (err == nil) || (ok && !bytes.Equal(ref, out)) {
			t.Fatalf("damaged input % x\nreference ok=%v; stream err=%v", enc, ok, err)
		}
	}
}
