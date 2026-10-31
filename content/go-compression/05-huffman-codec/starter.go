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
	lengths []int // 256 entries
}

// parseHeader reads: uvarint n, uvarint k, then k (symbol, length) byte pairs.
// It returns the header and how many bytes it used. ErrTruncated if the header
// is incomplete, ErrCorrupt if it is complete but invalid (see the task).
func parseHeader(data []byte) (huffHeader, int, error) {
	return huffHeader{}, 0, ErrTruncated
}

// HuffDecode reverses HuffEncode and rejects anything HuffEncode could not have produced.
func HuffDecode(enc []byte) ([]byte, error) { return nil, ErrTruncated }

func main() {
	enc := HuffEncode([]byte("abracadabra"))
	out, err := HuffDecode(enc)
	fmt.Printf("11 bytes -> % x (%d bytes)\n%q %v\n", enc, len(enc), out, err)
	enc[len(enc)-1] ^= 1
	_, err = HuffDecode(enc)
	fmt.Println("with a flipped padding bit:", err)
}
