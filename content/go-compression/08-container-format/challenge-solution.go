package main

import (
	"encoding/binary"
	"errors"
	"fmt"
	"hash/crc32"
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

// RLEEncode: tokens are literal runs (control 0..127 = 1..128 raw bytes follow) and
// repeats (control 128..255 = 2..129 copies of the one byte that follows).
// A run of 3 or more equal bytes becomes a repeat (at most 129 per token); everything
// else is collected into literals (at most 128 per token).
func RLEEncode(data []byte) []byte {
	var out, lit []byte
	flush := func() {
		if len(lit) > 0 {
			out = append(out, byte(len(lit)-1))
			out = append(out, lit...)
			lit = lit[:0]
		}
	}
	for i := 0; i < len(data); {
		r := 1
		for i+r < len(data) && data[i+r] == data[i] && r < 129 {
			r++
		}
		if r >= 3 {
			flush()
			out = append(out, byte(128+r-2), data[i])
			i += r
			continue
		}
		lit = append(lit, data[i])
		i++
		if len(lit) == 128 {
			flush()
		}
	}
	flush()
	return out
}

func RLEDecode(enc []byte) ([]byte, error) {
	var out []byte
	for i := 0; i < len(enc); {
		c := int(enc[i])
		i++
		if c < 128 {
			n := c + 1
			if i+n > len(enc) {
				return nil, ErrTruncated
			}
			out = append(out, enc[i:i+n]...)
			i += n
		} else {
			if i >= len(enc) {
				return nil, ErrTruncated
			}
			for k := 0; k < c-128+2; k++ {
				out = append(out, enc[i])
			}
			i++
		}
	}
	return out, nil
}

var ErrTooLarge = errors.New("output exceeds the limit")

type Limits struct {
	MaxOutput  int // bytes
	MaxRatio   int // output may not exceed MaxRatio * input bytes consumed (0 = no ratio check)
	RatioFloor int // the ratio is only enforced once the output is larger than this
}

func RLEDecodeLimited(enc []byte, lim Limits) ([]byte, error) {
	var out []byte
	for i := 0; i < len(enc); {
		c := int(enc[i])
		var n, need int
		if c < 128 {
			n, need = c+1, 1+c+1
		} else {
			n, need = c-128+2, 2
		}
		if i+need > len(enc) {
			return nil, ErrTruncated
		}
		total := len(out) + n
		consumed := i + need
		if total > lim.MaxOutput || (lim.MaxRatio > 0 && total > lim.RatioFloor && total > lim.MaxRatio*consumed) {
			return nil, ErrTooLarge
		}
		if c < 128 {
			out = append(out, enc[i+1:i+need]...)
		} else {
			for k := 0; k < n; k++ {
				out = append(out, enc[i+1])
			}
		}
		i += need
	}
	return out, nil
}

// Pack stores data raw (mode 0) or RLE-compressed (mode 1, only if strictly shorter).
func Pack(data []byte) []byte {
	if r := RLEEncode(data); len(r) < len(data) {
		return append([]byte{1}, r...)
	}
	return append([]byte{0}, data...)
}

func Unpack(enc []byte, lim Limits) ([]byte, error) {
	if len(enc) == 0 {
		return nil, ErrTruncated
	}
	switch enc[0] {
	case 0:
		if len(enc)-1 > lim.MaxOutput {
			return nil, ErrTooLarge
		}
		return append([]byte(nil), enc[1:]...), nil
	case 1:
		return RLEDecodeLimited(enc[1:], lim)
	}
	return nil, ErrCorrupt
}

var (
	ErrFormat   = errors.New("not a BMZ file")
	ErrVersion  = errors.New("unsupported version or feature")
	ErrChecksum = errors.New("chunk checksum mismatch")
)

// readHeader checks the magic "BMZ" and returns the version byte.
func readHeader(c []byte) (byte, error) {
	for i := 0; i < len(c) && i < 3; i++ {
		if c[i] != "BMZ"[i] {
			return 0, ErrFormat
		}
	}
	if len(c) < 4 {
		return 0, ErrTruncated
	}
	return c[3], nil
}

func lenErr(err error) error {
	if err == ErrShort {
		return ErrTruncated
	}
	return ErrCorrupt
}

func appendChunk(out, raw, enc []byte) []byte {
	out = AppendUvarint(out, uint64(len(raw)))
	out = AppendUvarint(out, uint64(len(enc)))
	out = append(out, enc...)
	return binary.BigEndian.AppendUint32(out, crc32.ChecksumIEEE(raw))
}

// Compress writes a version 1 file: magic, version, chunks, then a single 0 byte.
func Compress(data []byte, chunk int) []byte {
	out := []byte("BMZ\x01")
	for i := 0; i < len(data); i += chunk {
		c := data[i:min(i+chunk, len(data))]
		out = appendChunk(out, c, Pack(c))
	}
	return append(out, 0)
}

// decodeChunks reads chunks until the end marker. unpack decodes one chunk body
// and may use at most max bytes of output.
func decodeChunks(body []byte, maxOutput int, unpack func(enc []byte, max int) ([]byte, error)) ([]byte, error) {
	if maxOutput < 0 {
		maxOutput = 0
	}
	var out []byte
	pos := 0
	for {
		raw, n, err := Uvarint(body[pos:])
		if err != nil {
			return nil, lenErr(err)
		}
		pos += n
		if raw == 0 {
			if pos != len(body) {
				return nil, ErrCorrupt
			}
			return out, nil
		}
		if raw > uint64(maxOutput-len(out)) {
			return nil, ErrTooLarge
		}
		encLen, n, err := Uvarint(body[pos:])
		if err != nil {
			return nil, lenErr(err)
		}
		pos += n
		if encLen == 0 {
			return nil, ErrCorrupt
		}
		if encLen > uint64(len(body)-pos) {
			return nil, ErrTruncated
		}
		chunk, err := unpack(body[pos:pos+int(encLen)], int(raw))
		pos += int(encLen)
		if err != nil || uint64(len(chunk)) != raw {
			return nil, ErrCorrupt
		}
		if len(body)-pos < 4 {
			return nil, ErrTruncated
		}
		if binary.BigEndian.Uint32(body[pos:]) != crc32.ChecksumIEEE(chunk) {
			return nil, ErrChecksum
		}
		pos += 4
		out = append(out, chunk...)
	}
}

func Decompress(c []byte, maxOutput int) ([]byte, error) {
	v, err := readHeader(c)
	if err != nil {
		return nil, err
	}
	if v != 1 {
		return nil, ErrVersion
	}
	return decodeChunks(c[4:], maxOutput, func(enc []byte, max int) ([]byte, error) {
		return Unpack(enc, Limits{MaxOutput: max})
	})
}

const (
	FlagHuffman = 0x01
	FlagMeta    = 0x02
	maxMeta     = 1024
)

func CompressV2(data []byte, chunk int, meta []byte) []byte {
	flags := uint64(FlagHuffman)
	if len(meta) > 0 {
		flags |= FlagMeta
	}
	out := AppendUvarint([]byte("BMZ\x02"), flags)
	if len(meta) > 0 {
		out = append(AppendUvarint(out, uint64(len(meta))), meta...)
	}
	for i := 0; i < len(data); i += chunk {
		c := data[i:min(i+chunk, len(data))]
		best := append([]byte{0}, c...)
		if e := append([]byte{1}, RLEEncode(c)...); len(e) < len(best) {
			best = e
		}
		if e := append([]byte{2}, HuffEncode(c)...); len(e) < len(best) {
			best = e
		}
		out = appendChunk(out, c, best)
	}
	return append(out, 0)
}

func DecompressAny(c []byte, maxOutput int) ([]byte, error) {
	v, err := readHeader(c)
	if err != nil {
		return nil, err
	}
	switch v {
	case 1:
		return Decompress(c, maxOutput)
	case 2:
		flags, n, err := Uvarint(c[4:])
		if err != nil {
			return nil, lenErr(err)
		}
		pos := 4 + n
		if flags&^0x0F != 0 {
			return nil, ErrVersion
		}
		if flags&FlagMeta != 0 {
			ml, n, err := Uvarint(c[pos:])
			if err != nil {
				return nil, lenErr(err)
			}
			pos += n
			if ml > maxMeta {
				return nil, ErrTooLarge
			}
			if ml > uint64(len(c)-pos) {
				return nil, ErrTruncated
			}
			pos += int(ml)
		}
		huff := flags&FlagHuffman != 0
		return decodeChunks(c[pos:], maxOutput, func(enc []byte, max int) ([]byte, error) {
			if len(enc) > 0 && enc[0] == 2 {
				if !huff {
					return nil, ErrCorrupt
				}
				h, _, err := parseHeader(enc[1:])
				if err != nil {
					return nil, err
				}
				if h.n > uint64(max) {
					return nil, ErrTooLarge
				}
				return HuffDecode(enc[1:])
			}
			return Unpack(enc, Limits{MaxOutput: max})
		})
	}
	return nil, ErrVersion
}

func main() {
	data := []byte("hello hello hello hello, container!")
	v1 := Compress(data, 16)
	fmt.Println("v1 header:", v1[:4])
	// CompressV2 and DecompressAny are yours to write.
	v2 := CompressV2(data, 16, []byte("note"))
	out, err := DecompressAny(v2, 1<<20)
	fmt.Printf("%q %v\n", out, err)
}
