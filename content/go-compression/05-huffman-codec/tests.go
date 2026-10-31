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

type hcase struct{ in, enc string }

var huffKnown = []hcase{{"", "0000"}, {"61", "0101610100"}, {"61616161", "0401610100"}, {"6162726163616461627261", "0b056101620363046404720269cf68"}, {"6162", "02026101620140"}, {"68656c6c6f2c2068656c6c6f2c2068656c6c6f20776f726c64", "190920032c046404650368036c026f03720477048c2e28c2e28c2afbc680"}}

func TestHuffDecode_known_answers(t *testing.T) {
	for _, c := range huffKnown {
		got, err := HuffDecode(unhex(c.enc))
		if err != nil || !bytes.Equal(got, unhex(c.in)) {
			t.Errorf("HuffDecode(%s) = %q, %v; want %q", c.enc, got, err, unhex(c.in))
		}
		if enc := hex.EncodeToString(HuffEncode(unhex(c.in))); enc != c.enc {
			t.Errorf("HuffEncode (given) disagrees with the vector: %s vs %s", enc, c.enc)
		}
	}
}

func TestHuffDecode_round_trip_and_reference(t *testing.T) {
	for i, d := range huffInputs() {
		enc := HuffEncode(d)
		got, err := HuffDecode(enc)
		if err != nil || !bytes.Equal(got, d) {
			t.Fatalf("input %d (%d bytes): round trip failed, err=%v", i, len(d), err)
		}
		if ref, _, _, ok := refHuff(enc); !ok || !bytes.Equal(ref, d) {
			t.Fatalf("input %d: the test's reference decoder disagrees (ok=%v)", i, ok)
		}
	}
}

func TestHuffDecode_every_proper_prefix_is_truncated(t *testing.T) {
	for _, d := range [][]byte{nil, []byte("abracadabra"), []byte("zzzzzzzzzz"), skewed(&tRand{s: 4}, 120, 9)} {
		enc := HuffEncode(d)
		for cut := 0; cut < len(enc); cut++ {
			if _, err := HuffDecode(enc[:cut]); !errors.Is(err, ErrTruncated) {
				t.Fatalf("%d-byte input cut to %d bytes: err = %v; want ErrTruncated", len(d), cut, err)
			}
		}
	}
}

func TestHuffDecode_rejects_invalid_headers(t *testing.T) {
	two := []byte{2, 2, 'a', 1, 'b', 1, 0x40} // n=2, a=0, b=1: "ab"
	if got, err := HuffDecode(two); err != nil || string(got) != "ab" {
		t.Fatalf("baseline stream: %q, %v", got, err)
	}
	bad := map[string][]byte{
		"more than 256 symbols":      {2, 0xAC, 0x02},
		"symbols out of order":       {2, 2, 'b', 1, 'a', 1, 0x40},
		"duplicate symbol":           {2, 2, 'a', 1, 'a', 1, 0x40},
		"length zero":                {2, 2, 'a', 0, 'b', 1, 0x40},
		"length above the maximum":   {2, 2, 'a', 49, 'b', 1, 0x40},
		"oversubscribed code":        {2, 3, 'a', 1, 'b', 1, 'c', 1, 0x40},
		"incomplete code":            {2, 2, 'a', 1, 'b', 2, 0x40},
		"one symbol with length two": {2, 1, 'a', 2, 0x00},
		"symbols but no symbol list": {5, 0, 0x00},
		"non-canonical n":            {0x82, 0x00, 2, 'a', 1, 'b', 1, 0x40},
		"non-zero padding":           {2, 2, 'a', 1, 'b', 1, 0x41},
		"extra trailing byte":        {2, 2, 'a', 1, 'b', 1, 0x40, 0x00},
		"payload where n is zero":    {0, 0, 0x00},
	}
	for name, enc := range bad {
		if got, err := HuffDecode(enc); !errors.Is(err, ErrCorrupt) || got != nil {
			t.Errorf("%s: HuffDecode(% x) = %q, %v; want nil, ErrCorrupt", name, enc, got, err)
		}
	}
	if _, err := HuffDecode([]byte{3, 1, 'a', 1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}); err == nil {
		t.Errorf("a one-symbol stream containing a 1 bit has no valid code and must fail")
	}
}

func TestHuffDecode_lying_lengths_do_not_allocate(t *testing.T) {
	huge := binary.AppendUvarint(nil, 1<<40)
	huge = append(huge, 2, 'a', 1, 'b', 1, 0x40)
	var err error
	a := allocated(func() { _, err = HuffDecode(huge) })
	if !errors.Is(err, ErrTruncated) {
		t.Errorf("a header claiming 2^40 symbols with one payload byte: err = %v; want ErrTruncated", err)
	}
	if a > 1<<20 {
		t.Errorf("rejecting that header allocated %d bytes; check the claimed length against the payload before allocating", a)
	}
	// Claiming fewer symbols than the payload holds must never invent data.
	enc := HuffEncode([]byte("abracadabra"))
	enc[0]--
	if got, err := HuffDecode(enc); err == nil && !bytes.HasPrefix([]byte("abracadabra"), got) {
		t.Errorf("n lowered by one: decoded %q", got)
	}
}

func TestHuffDecode_agrees_with_reference_on_damaged_input(t *testing.T) {
	r := &tRand{s: 1234}
	ins := huffInputs()
	for i := 0; i < 1500; i++ {
		enc := mutate(r, HuffEncode(ins[r.intn(len(ins))]))
		ref, _, _, ok := refHuff(enc)
		got, err := func() (g []byte, e error) {
			defer func() {
				if p := recover(); p != nil {
					e = fmt.Errorf("panic: %v", p)
				}
			}()
			return HuffDecode(enc)
		}()
		if ok != (err == nil) || (ok && !bytes.Equal(ref, got)) {
			t.Fatalf("damaged input % x\nreference ok=%v; HuffDecode err=%v", enc, ok, err)
		}
	}
}
