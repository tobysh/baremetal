package main

import (
	"errors"
	"fmt"
)

var ErrShort = errors.New("input ended early")

var ErrCorrupt = errors.New("corrupt data")

// BitWriter packs bits most-significant-first.
type BitWriter struct {
	buf   []byte
	nbits int
}

// WriteBits appends the low n bits of v (0 <= n <= 64), highest of those bits first.
func (w *BitWriter) WriteBits(v uint64, n int) {
	for i := n - 1; i >= 0; i-- {
		if w.nbits%8 == 0 {
			w.buf = append(w.buf, 0)
		}
		w.buf[len(w.buf)-1] |= byte(v>>uint(i)&1) << uint(7-w.nbits%8)
		w.nbits++
	}
}

// Len is the number of bits written.
func (w *BitWriter) Len() int { return w.nbits }

// Bytes returns a copy of the bytes so far; the last byte is padded with zero bits.
func (w *BitWriter) Bytes() []byte { return append([]byte(nil), w.buf...) }

type BitReader struct {
	data []byte
	pos  int
}

func NewBitReader(data []byte) *BitReader { return &BitReader{data: data} }

// ReadBits reads n bits (0 <= n <= 64). If fewer than n remain it returns ErrShort and consumes nothing.
func (r *BitReader) ReadBits(n int) (uint64, error) {
	if n > r.Remaining() {
		return 0, ErrShort
	}
	var v uint64
	for i := 0; i < n; i++ {
		v = v<<1 | uint64(r.data[r.pos/8]>>uint(7-r.pos%8)&1)
		r.pos++
	}
	return v, nil
}

func (r *BitReader) Pos() int       { return r.pos }
func (r *BitReader) Remaining() int { return len(r.data)*8 - r.pos }

var ErrTruncated = errors.New("input is truncated")

var (
	ErrOverflow     = errors.New("varint overflows 64 bits")
	ErrNonCanonical = errors.New("varint is not the shortest encoding")
)

func AppendUvarint(dst []byte, v uint64) []byte {
	for v >= 0x80 {
		dst = append(dst, byte(v)|0x80)
		v >>= 7
	}
	return append(dst, byte(v))
}

func Uvarint(b []byte) (uint64, int, error) {
	var v uint64
	for i := 0; i < len(b); i++ {
		c := b[i]
		if i == 9 && c > 1 {
			return 0, 0, ErrOverflow
		}
		v |= uint64(c&0x7f) << (7 * uint(i))
		if c < 0x80 {
			if i > 0 && c == 0 {
				return 0, 0, ErrNonCanonical
			}
			return v, i + 1, nil
		}
	}
	return 0, 0, ErrShort
}

func AppendVarint(dst []byte, v int64) []byte {
	return AppendUvarint(dst, uint64(v<<1)^uint64(v>>63))
}

func Varint(b []byte) (int64, int, error) {
	u, n, err := Uvarint(b)
	if err != nil {
		return 0, 0, err
	}
	return int64(u>>1) ^ -int64(u&1), n, nil
}

const MaxCodeLen = 48

var ErrBadLengths = errors.New("invalid code lengths")

type Code struct {
	Bits uint64
	Len  int
}

// BuildLengths: repeatedly merge the two lightest nodes, ordered by (weight, smallest symbol inside).
func BuildLengths(freq []int) []int {
	lengths := make([]int, len(freq))
	type node struct{ w, min, l, r int }
	var nodes []node
	var active []int
	for s, f := range freq {
		if f > 0 {
			nodes = append(nodes, node{f, s, -1, -1})
			active = append(active, len(nodes)-1)
		}
	}
	if len(active) == 0 {
		return lengths
	}
	if len(active) == 1 {
		lengths[nodes[active[0]].min] = 1
		return lengths
	}
	pick := func() int {
		best := 0
		for i := 1; i < len(active); i++ {
			x, y := nodes[active[i]], nodes[active[best]]
			if x.w < y.w || (x.w == y.w && x.min < y.min) {
				best = i
			}
		}
		n := active[best]
		active = append(active[:best], active[best+1:]...)
		return n
	}
	for len(active) > 1 {
		a, b := pick(), pick()
		m := nodes[a].min
		if nodes[b].min < m {
			m = nodes[b].min
		}
		nodes = append(nodes, node{nodes[a].w + nodes[b].w, m, a, b})
		active = append(active, len(nodes)-1)
	}
	var walk func(n, d int)
	walk = func(n, d int) {
		x := nodes[n]
		if x.l < 0 {
			lengths[x.min] = d
			return
		}
		walk(x.l, d+1)
		walk(x.r, d+1)
	}
	walk(active[0], 0)
	return lengths
}

func CanonicalCodes(lengths []int) ([]Code, error) {
	var count [MaxCodeLen + 1]int
	var kraft uint64
	for _, l := range lengths {
		if l < 0 || l > MaxCodeLen {
			return nil, ErrBadLengths
		}
		if l > 0 {
			count[l]++
			kraft += 1 << uint(MaxCodeLen-l)
			if kraft > 1<<MaxCodeLen {
				return nil, ErrBadLengths
			}
		}
	}
	var next [MaxCodeLen + 1]uint64
	code := uint64(0)
	for l := 1; l <= MaxCodeLen; l++ {
		code = (code + uint64(count[l-1])) << 1
		next[l] = code
	}
	codes := make([]Code, len(lengths))
	for s, l := range lengths {
		if l > 0 {
			codes[s] = Code{next[l], l}
			next[l]++
		}
	}
	return codes, nil
}

// HuffEncode: uvarint len(data), uvarint k (symbols used), k pairs (symbol, length)
// in ascending symbol order, then the canonical-code bits, zero-padded to a byte.
func HuffEncode(data []byte) []byte {
	var freq [256]int
	for _, b := range data {
		freq[b]++
	}
	lengths := BuildLengths(freq[:])
	codes, _ := CanonicalCodes(lengths)
	k := 0
	for _, l := range lengths {
		if l > 0 {
			k++
		}
	}
	out := AppendUvarint(AppendUvarint(nil, uint64(len(data))), uint64(k))
	for s, l := range lengths {
		if l > 0 {
			out = append(out, byte(s), byte(l))
		}
	}
	var w BitWriter
	for _, b := range data {
		w.WriteBits(codes[b].Bits, codes[b].Len)
	}
	return append(out, w.Bytes()...)
}

type huffHeader struct {
	n       uint64
	lengths []int
}

// parseHeader: uvarint n, uvarint k, then k (symbol, length) byte pairs.
// ErrTruncated when the header is incomplete, ErrCorrupt when it is complete but invalid.
func parseHeader(data []byte) (huffHeader, int, error) {
	var h huffHeader
	n, c, err := Uvarint(data)
	if err == ErrShort {
		return h, 0, ErrTruncated
	} else if err != nil {
		return h, 0, ErrCorrupt
	}
	pos := c
	k, c, err := Uvarint(data[pos:])
	if err == ErrShort {
		return h, 0, ErrTruncated
	} else if err != nil || k > 256 {
		return h, 0, ErrCorrupt
	}
	pos += c
	if uint64(len(data)-pos) < 2*k {
		return h, 0, ErrTruncated
	}
	h.n = n
	h.lengths = make([]int, 256)
	prev := -1
	var kraft uint64
	for i := uint64(0); i < k; i++ {
		s, l := int(data[pos]), int(data[pos+1])
		pos += 2
		if s <= prev || l < 1 || l > MaxCodeLen {
			return h, 0, ErrCorrupt
		}
		prev = s
		h.lengths[s] = l
		kraft += 1 << uint(MaxCodeLen-l)
	}
	switch {
	case k == 0:
		if n != 0 {
			return h, 0, ErrCorrupt
		}
	case k == 1:
		if h.lengths[prev] != 1 {
			return h, 0, ErrCorrupt
		}
	case kraft != 1<<MaxCodeLen:
		return h, 0, ErrCorrupt
	}
	return h, pos, nil
}

type canonTable struct {
	count  [MaxCodeLen + 1]int
	first  [MaxCodeLen + 1]uint64
	offset [MaxCodeLen + 1]int
	syms   []byte
}

func buildTable(lengths []int) *canonTable {
	t := &canonTable{}
	for _, l := range lengths {
		if l > 0 {
			t.count[l]++
		}
	}
	code, off := uint64(0), 0
	for l := 1; l <= MaxCodeLen; l++ {
		code = (code + uint64(t.count[l-1])) << 1
		t.first[l] = code
		t.offset[l] = off
		off += t.count[l]
	}
	for l := 1; l <= MaxCodeLen; l++ {
		for s, sl := range lengths {
			if sl == l {
				t.syms = append(t.syms, byte(s))
			}
		}
	}
	return t
}

// lookup reports the symbol for the clen-bit code, if it is one.
func (t *canonTable) lookup(code uint64, clen int) (byte, bool) {
	if t.count[clen] > 0 && code >= t.first[clen] && code-t.first[clen] < uint64(t.count[clen]) {
		return t.syms[t.offset[clen]+int(code-t.first[clen])], true
	}
	return 0, false
}

func HuffDecode(enc []byte) ([]byte, error) {
	h, used, err := parseHeader(enc)
	if err != nil {
		return nil, err
	}
	payload := enc[used:]
	if h.n > uint64(len(payload))*8 { // every symbol costs at least one bit
		return nil, ErrTruncated
	}
	t := buildTable(h.lengths)
	r := NewBitReader(payload)
	out := make([]byte, 0, h.n)
	for uint64(len(out)) < h.n {
		var code uint64
		for clen := 1; ; clen++ {
			if clen > MaxCodeLen {
				return nil, ErrCorrupt
			}
			b, err := r.ReadBits(1)
			if err != nil {
				return nil, ErrTruncated
			}
			code = code<<1 | b
			if s, ok := t.lookup(code, clen); ok {
				out = append(out, s)
				break
			}
		}
	}
	if (r.Pos()+7)/8 != len(payload) {
		return nil, ErrCorrupt
	}
	if rest, _ := r.ReadBits(r.Remaining()); rest != 0 {
		return nil, ErrCorrupt
	}
	return out, nil
}

// StreamDecoder decodes a HuffEncode stream fed in arbitrary pieces.
type StreamDecoder struct {
	// your state here
}

func NewStreamDecoder() *StreamDecoder { return &StreamDecoder{} }

// Write consumes the next chunk (any size, possibly empty) and returns the
// symbols that became decodable. After an error every later call returns the same error.
func (d *StreamDecoder) Write(p []byte) ([]byte, error) { return nil, nil }

// Close reports whether the stream ended exactly where it should:
// nil, ErrTruncated (ended early) or the error already returned by Write.
func (d *StreamDecoder) Close() error { return nil }

func main() {
	enc := HuffEncode([]byte("abracadabra abracadabra"))
	d := NewStreamDecoder()
	for i := 0; i < len(enc); i += 3 {
		out, err := d.Write(enc[i:min(i+3, len(enc))])
		fmt.Printf("chunk at %2d -> %q %v\n", i, out, err)
	}
	fmt.Println("close:", d.Close())
}
