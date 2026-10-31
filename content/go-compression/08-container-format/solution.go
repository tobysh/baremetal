package main

import (
	"encoding/binary"
	"errors"
	"fmt"
	"hash/crc32"
)

var ErrShort = errors.New("input ended early")

var ErrCorrupt = errors.New("corrupt data")

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

func main() {
	data := []byte("hello hello hello hello, container!")
	file := Compress(data, 16)
	fmt.Printf("%d bytes -> %d bytes: % x\n", len(data), len(file), file[:8])
	out, err := Decompress(file, 1<<20)
	fmt.Printf("%q %v\n", out, err)
	file[len(file)-3] ^= 0x10
	_, err = Decompress(file, 1<<20)
	fmt.Println("after damage:", err)
}
