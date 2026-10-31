package main

import (
	"bytes"
	"encoding/binary"
	"encoding/hex"
	"errors"
	"fmt"
	"hash/crc32"
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

type contCase struct {
	in    string
	chunk int
	enc   string
}

var contV1 = []contCase{{"", 8, "424d5a0100"},
	{"68656c6c6f2068656c6c6f2068656c6c6f212068656c6c6f2068656c6c6f2068656c6c6f212068656c6c6f2068656c6c6f2068656c6c6f212068656c6c6f2068656c6c6f2068656c6c6f212068656c6c6f2068656c6c6f2068656c6c6f212078787878787878787878787878787878787878787878787878787878787878787878787878787878656e64", 32, "424d5a0120210068656c6c6f2068656c6c6f2068656c6c6f212068656c6c6f2068656c6c6f2068cf9569e5202100656c6c6f212068656c6c6f2068656c6c6f2068656c6c6f212068656c6c6f2068f9942db4202100656c6c6f2068656c6c6f212068656c6c6f2068656c6c6f2068656c6c6f21207820e437532003019e7800ce3d880a0701857802656e6492b89ecc00"},
	{"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f3031", 16, "424d5a01101100000102030405060708090a0b0c0d0e0fcecee288101100101112131415161718191a1b1c1d1e1ff4a7fd67101100202122232425262728292a2b2c2d2e2fba1cdd560203003031cf41243600"},
	{"616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161", 100, "424d5a01640301e261af707a64640301e261af707a64640301e261af707a6400"},
	{"61627261636164616272616162726163616461627261616272616361646162726161627261636164616272616162726163616461627261616272616361646162726161627261636164616272616162726163616461627261616272616361646162726161627261636164616272616162726163616461627261616272616361646162726161627261636164616272616162726163616461627261616272616361646162726161627261636164616272616162726163616461627261616272616361646162726161627261636164616272616162726163616461627261", 64, "424d5a0140410061627261636164616272616162726163616461627261616272616361646162726161627261636164616272616162726163616461627261616272616361646162e8434189404100726161627261636164616272616162726163616461627261616272616361646162726161627261636164616272616162726163616461627261616272616361648ffc957240410061627261616272616361646162726161627261636164616272616162726163616461627261616272616361646162726161627261636164616272616162726163e942eca91c1d00616461627261616272616361646162726161627261636164616272618c33855100"},
}

func tChunk(raw, enc []byte) []byte {
	out := binary.AppendUvarint(nil, uint64(len(raw)))
	out = binary.AppendUvarint(out, uint64(len(enc)))
	out = append(out, enc...)
	return binary.BigEndian.AppendUint32(out, crc32.ChecksumIEEE(raw))
}

func v1file(chunks ...[]byte) []byte {
	return append(append([]byte("BMZ\x01"), bytes.Join(chunks, nil)...), 0)
}

func TestCompress_known_answers(t *testing.T) {
	for _, c := range contV1 {
		if got := hex.EncodeToString(Compress(unhex(c.in), c.chunk)); got != c.enc {
			t.Errorf("Compress(%d bytes, chunk %d):\n got %s\nwant %s", len(c.in)/2, c.chunk, got, c.enc)
		}
	}
}

func TestDecompress_known_answers_and_round_trips(t *testing.T) {
	for _, c := range contV1 {
		got, err := Decompress(unhex(c.enc), 1<<20)
		if err != nil || !bytes.Equal(got, unhex(c.in)) {
			t.Errorf("Decompress(known file for %d bytes): %d bytes, %v", len(c.in)/2, len(got), err)
		}
	}
	r := &tRand{s: 40}
	for i := 0; i < 80; i++ {
		d := skewed(r, r.intn(1500), 1+r.intn(30))
		chunk := 1 + r.intn(300)
		got, err := Decompress(Compress(d, chunk), len(d))
		if err != nil || !bytes.Equal(got, d) {
			t.Fatalf("round trip %d (len %d, chunk %d): err=%v", i, len(d), chunk, err)
		}
	}
}

func TestDecompress_every_proper_prefix_is_truncated(t *testing.T) {
	file := Compress(skewed(&tRand{s: 1}, 400, 6), 90)
	for cut := 0; cut < len(file); cut++ {
		if _, err := Decompress(file[:cut], 1<<20); !errors.Is(err, ErrTruncated) {
			t.Fatalf("file cut to %d of %d bytes: err = %v; want ErrTruncated", cut, len(file), err)
		}
	}
}

func TestDecompress_no_single_bit_flip_goes_unnoticed(t *testing.T) {
	d := skewed(&tRand{s: 3}, 300, 5)
	file := Compress(d, 100)
	for i := 0; i < len(file); i++ {
		for bit := 0; bit < 8; bit++ {
			bad := append([]byte{}, file...)
			bad[i] ^= 1 << uint(bit)
			got, err := Decompress(bad, 1<<20)
			if err == nil && !bytes.Equal(got, d) {
				t.Fatalf("flipping bit %d of byte %d produced wrong data without an error", bit, i)
			}
		}
	}
}

func TestDecompress_header_errors(t *testing.T) {
	cases := []struct {
		name string
		in   []byte
		want error
	}{
		{"empty", nil, ErrTruncated},
		{"start of the magic", []byte("BM"), ErrTruncated},
		{"wrong magic", []byte("PNG\x01\x00"), ErrFormat},
		{"wrong magic early", []byte("X"), ErrFormat},
		{"future version", []byte("BMZ\x09\x00"), ErrVersion},
		{"version zero", []byte("BMZ\x00\x00"), ErrVersion},
		{"data after the end marker", append(Compress([]byte("hi"), 8), 0), ErrCorrupt},
		{"zero encoded length", []byte("BMZ\x01\x02\x00"), ErrCorrupt},
	}
	for _, c := range cases {
		if _, err := Decompress(c.in, 1<<20); !errors.Is(err, c.want) {
			t.Errorf("%s: err = %v; want %v", c.name, err, c.want)
		}
	}
}

func TestDecompress_lying_chunks(t *testing.T) {
	raw := []byte("hello")
	good := append([]byte{0}, raw...)
	if got, err := Decompress(v1file(tChunk(raw, good)), 100); err != nil || string(got) != "hello" {
		t.Fatalf("hand-built file: %q, %v", got, err)
	}
	badCRC := tChunk(raw, good)
	badCRC[len(badCRC)-1] ^= 1
	longer := binary.AppendUvarint(nil, 3) // declares 3 raw bytes but the body holds 5
	longer = append(binary.AppendUvarint(longer, 6), good...)
	longer = binary.BigEndian.AppendUint32(longer, crc32.ChecksumIEEE(raw))
	declaresMore := binary.AppendUvarint(nil, 9)
	declaresMore = append(binary.AppendUvarint(declaresMore, 6), good...)
	declaresMore = binary.BigEndian.AppendUint32(declaresMore, crc32.ChecksumIEEE(raw))
	huge := binary.AppendUvarint(nil, 3_000_000_000)
	huge = append(binary.AppendUvarint(huge, 2), 1, 0x00)
	hugeEnc := binary.AppendUvarint(nil, 5)
	hugeEnc = binary.AppendUvarint(hugeEnc, 4_000_000_000)
	cases := []struct {
		name string
		file []byte
		max  int
		want error
	}{
		{"checksum", v1file(badCRC), 100, ErrChecksum},
		{"chunk longer than declared", v1file(longer), 100, ErrCorrupt},
		{"chunk shorter than declared", v1file(declaresMore), 100, ErrCorrupt},
		{"raw length claims 3 GB", v1file(huge), 1 << 20, ErrTooLarge},
		{"encoded length claims 4 GB", v1file(hugeEnc), 1 << 20, ErrTruncated},
		{"total over the limit by one", v1file(tChunk(raw, good), tChunk(raw, good)), 9, ErrTooLarge},
		{"corrupt RLE inside a chunk", v1file(tChunk(raw, []byte{1, 0x09, 'x'})), 100, ErrCorrupt},
		{"unknown chunk method", v1file(tChunk(raw, append([]byte{5}, raw...))), 100, ErrCorrupt},
	}
	for _, c := range cases {
		var err error
		a := allocated(func() { _, err = Decompress(c.file, c.max) })
		if !errors.Is(err, c.want) {
			t.Errorf("%s: err = %v; want %v", c.name, err, c.want)
		}
		if a > 4<<20 {
			t.Errorf("%s: allocated %d bytes for a %d-byte file", c.name, a, len(c.file))
		}
	}
	if got, err := Decompress(v1file(tChunk(raw, good), tChunk(raw, good)), 10); err != nil || len(got) != 10 {
		t.Errorf("exactly maxOutput bytes must pass: %d, %v", len(got), err)
	}
	if got, err := Decompress(Compress(nil, 10), 0); err != nil || len(got) != 0 {
		t.Errorf("empty file with maxOutput 0: %v, %v", got, err)
	}
}

type v2Case struct {
	in, meta string
	chunk    int
	enc      string
}

var contV2 = []v2Case{{"", "", 8, "424d5a020100"},
	{"68656c6c6f2068656c6c6f2068656c6c6f212068656c6c6f2068656c6c6f2068656c6c6f212068656c6c6f2068656c6c6f2068656c6c6f212068656c6c6f2068656c6c6f2068656c6c6f212068656c6c6f2068656c6c6f2068656c6c6f212078787878787878787878787878787878787878787878787878787878787878787878787878787878656e64", "6d6164652062792074657374", 32, "424d5a02030c6d6164652062792074657374201902200620032103650368026c026f0332f865f0cbec32f865f0cf9569e5201a02200620032103650368036c026f02c0d9f033e067c0d9f03380f9942db4201c02200720032104650368036c026f027804a0cd41e9a8335066a0f4f020e437532003019e7800ce3d880a0701857802656e6492b89ecc00"},
	{"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f3031", "", 16, "424d5a0201101100000102030405060708090a0b0c0d0e0fcecee288101100101112131415161718191a1b1c1d1e1ff4a7fd67101100202122232425262728292a2b2c2d2e2fba1cdd560203003031cf41243600"},
	{"616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161", "78", 100, "424d5a02030178640301e261af707a64640301e261af707a64640301e261af707a6400"},
	{"61627261636164616272616162726163616461627261616272616361646162726161627261636164616272616162726163616461627261616272616361646162726161627261636164616272616162726163616461627261616272616361646162726161627261636164616272616162726163616461627261616272616361646162726161627261636164616272616162726163616461627261616272616361646162726161627261636164616272616162726163616461627261616272616361646162726161627261636164616272616162726163616461627261", "6d657461", 64, "424d5a0203046d657461401e024005610162036303640372034eac9c9d59393ab2727564e4eac9c9d590e8434189401e024005610162036304640472028d39ed1a73da34e7b469cf68d39ed1a73c8ffc9572401e0240056101620363046404720268d39ed1a73da34e7b469cf68d39ed1a70e942eca91c15021c05610162036304640472027b469cf68d39ed008c33855100"},
}
var contV1Gold = []contCase{{"", 8, "424d5a0100"},
	{"68656c6c6f2068656c6c6f2068656c6c6f212068656c6c6f2068656c6c6f2068656c6c6f212068656c6c6f2068656c6c6f2068656c6c6f212068656c6c6f2068656c6c6f2068656c6c6f212068656c6c6f2068656c6c6f2068656c6c6f212078787878787878787878787878787878787878787878787878787878787878787878787878787878656e64", 32, "424d5a0120210068656c6c6f2068656c6c6f2068656c6c6f212068656c6c6f2068656c6c6f2068cf9569e5202100656c6c6f212068656c6c6f2068656c6c6f2068656c6c6f212068656c6c6f2068f9942db4202100656c6c6f2068656c6c6f212068656c6c6f2068656c6c6f2068656c6c6f21207820e437532003019e7800ce3d880a0701857802656e6492b89ecc00"},
	{"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f3031", 16, "424d5a01101100000102030405060708090a0b0c0d0e0fcecee288101100101112131415161718191a1b1c1d1e1ff4a7fd67101100202122232425262728292a2b2c2d2e2fba1cdd560203003031cf41243600"},
	{"616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161", 100, "424d5a01640301e261af707a64640301e261af707a64640301e261af707a6400"},
	{"61627261636164616272616162726163616461627261616272616361646162726161627261636164616272616162726163616461627261616272616361646162726161627261636164616272616162726163616461627261616272616361646162726161627261636164616272616162726163616461627261616272616361646162726161627261636164616272616162726163616461627261616272616361646162726161627261636164616272616162726163616461627261616272616361646162726161627261636164616272616162726163616461627261", 64, "424d5a0140410061627261636164616272616162726163616461627261616272616361646162726161627261636164616272616162726163616461627261616272616361646162e8434189404100726161627261636164616272616162726163616461627261616272616361646162726161627261636164616272616162726163616461627261616272616361648ffc957240410061627261616272616361646162726161627261636164616272616162726163616461627261616272616361646162726161627261636164616272616162726163e942eca91c1d00616461627261616272616361646162726161627261636164616272618c33855100"},
}

// oldReader is the version 1 reader exactly as shipped before version 2 existed.
func oldReader(c []byte, max int) ([]byte, error) {
	return Decompress(c, max)
}

func TestCompatibility_new_reader_opens_every_old_file(t *testing.T) {
	for _, c := range contV1Gold {
		got, err := DecompressAny(unhex(c.enc), 1<<20)
		if err != nil || !bytes.Equal(got, unhex(c.in)) {
			t.Errorf("version 1 file for %d bytes: %d bytes back, %v", len(c.in)/2, len(got), err)
		}
		if enc := hex.EncodeToString(Compress(unhex(c.in), c.chunk)); enc != c.enc {
			t.Errorf("Compress must keep writing byte-identical version 1 files")
		}
	}
}

func TestCompatibility_version2_known_answers(t *testing.T) {
	for _, c := range contV2 {
		in, meta := unhex(c.in), unhex(c.meta)
		if got := hex.EncodeToString(CompressV2(in, c.chunk, meta)); got != c.enc {
			t.Errorf("CompressV2(%d bytes, chunk %d, meta %q):\n got %s\nwant %s", len(in), c.chunk, meta, got, c.enc)
		}
		got, err := DecompressAny(unhex(c.enc), 1<<20)
		if err != nil || !bytes.Equal(got, in) {
			t.Errorf("DecompressAny(version 2 file for %d bytes): %d bytes, %v", len(in), len(got), err)
		}
	}
}

func TestCompatibility_old_reader_refuses_new_files_cleanly(t *testing.T) {
	for _, c := range contV2 {
		if _, err := oldReader(unhex(c.enc), 1<<20); !errors.Is(err, ErrVersion) {
			t.Errorf("the old reader given a version 2 file: err = %v; it must answer ErrVersion, not misread it", err)
		}
	}
}

func TestCompatibility_round_trip_and_prefixes(t *testing.T) {
	r := &tRand{s: 50}
	for i := 0; i < 60; i++ {
		d := skewed(r, r.intn(1200), 1+r.intn(25))
		var meta []byte
		if i%3 == 0 {
			meta = bytes.Repeat([]byte{'m'}, 1+r.intn(1024))
		}
		file := CompressV2(d, 1+r.intn(200), meta)
		got, err := DecompressAny(file, len(d))
		if err != nil || !bytes.Equal(got, d) {
			t.Fatalf("round trip %d: err=%v", i, err)
		}
		if i%10 == 0 {
			for cut := 0; cut < len(file); cut++ {
				if _, err := DecompressAny(file[:cut], 1<<20); !errors.Is(err, ErrTruncated) {
					t.Fatalf("v2 file cut to %d of %d: err = %v; want ErrTruncated", cut, len(file), err)
				}
			}
		}
	}
	d := skewed(&tRand{s: 9}, 300, 6)
	file := CompressV2(d, 100, []byte("note"))
	for i := range file {
		for bit := 0; bit < 8; bit++ {
			bad := append([]byte{}, file...)
			bad[i] ^= 1 << uint(bit)
			if got, err := DecompressAny(bad, 1<<20); err == nil && !bytes.Equal(got, d) {
				t.Fatalf("bit %d of byte %d flipped in a v2 file: wrong data, no error", bit, i)
			}
		}
	}
}

func TestCompatibility_flags_and_metadata_rules(t *testing.T) {
	body := append([]byte{}, tChunk([]byte("hey"), []byte{0, 'h', 'e', 'y'})...)
	build := func(flags byte, pre []byte) []byte {
		f := append([]byte("BMZ\x02"), flags)
		f = append(f, pre...)
		f = append(f, body...)
		return append(f, 0)
	}
	if got, err := DecompressAny(build(0x00, nil), 100); err != nil || string(got) != "hey" {
		t.Errorf("flags 0 (no Huffman, no metadata): %q, %v", got, err)
	}
	if got, err := DecompressAny(build(0x0C, nil), 100); err != nil || string(got) != "hey" {
		t.Errorf("unknown OPTIONAL flags (0x0c) must be ignored: %q, %v", got, err)
	}
	for _, f := range []byte{0x10, 0x20, 0x80, 0x11} {
		if _, err := DecompressAny(build(f, nil), 100); !errors.Is(err, ErrVersion) {
			t.Errorf("unknown CRITICAL flag 0x%02x: err = %v; want ErrVersion", f, err)
		}
	}
	if _, err := DecompressAny(append([]byte("BMZ\x02\x80\x01"), 0), 100); !errors.Is(err, ErrVersion) {
		t.Errorf("flags value 128 (a two-byte varint) is critical: %v", err)
	}
	meta := append([]byte{3}, "abc"...)
	if got, err := DecompressAny(build(FlagMeta, meta), 100); err != nil || string(got) != "hey" {
		t.Errorf("metadata must be skipped: %q, %v", got, err)
	}
	if _, err := DecompressAny(build(FlagMeta, binary.AppendUvarint(nil, 1025)), 100); !errors.Is(err, ErrTooLarge) {
		t.Errorf("metadata over 1024 bytes: %v; want ErrTooLarge", err)
	}
	if _, err := DecompressAny(build(FlagMeta, binary.AppendUvarint(nil, 1024)), 100); !errors.Is(err, ErrCorrupt) && !errors.Is(err, ErrTruncated) {
		t.Errorf("1024 bytes of metadata claimed but absent should be a truncation/corruption error, got %v", err)
	}
	for _, v := range []byte{0, 3, 200} {
		if _, err := DecompressAny([]byte{'B', 'M', 'Z', v, 0}, 100); !errors.Is(err, ErrVersion) {
			t.Errorf("version %d: %v; want ErrVersion", v, err)
		}
	}
	huff := tChunk([]byte("aaaa"), append([]byte{2}, HuffEncode([]byte("aaaa"))...))
	with := append(append([]byte("BMZ\x02\x01"), huff...), 0)
	without := append(append([]byte("BMZ\x02\x00"), huff...), 0)
	if got, err := DecompressAny(with, 100); err != nil || string(got) != "aaaa" {
		t.Errorf("Huffman chunk with the flag set: %q, %v", got, err)
	}
	if _, err := DecompressAny(without, 100); !errors.Is(err, ErrCorrupt) {
		t.Errorf("Huffman chunk without FlagHuffman: %v; want ErrCorrupt", err)
	}
}

func TestCompatibility_limits_hold_in_version2(t *testing.T) {
	bombHdr := binary.AppendUvarint(nil, 1<<40)
	bombHdr = append(bombHdr, 2, 'a', 1, 'b', 1, 0x40)
	f := append([]byte("BMZ\x02\x01"), tChunk([]byte("ab"), append([]byte{2}, bombHdr...))...)
	f = append(f, 0)
	var err error
	a := allocated(func() { _, err = DecompressAny(f, 1<<20) })
	if err == nil || a > 4<<20 {
		t.Errorf("Huffman chunk claiming 2^40 symbols: err=%v, allocated %d", err, a)
	}
	big := CompressV2(bytes.Repeat([]byte{'x'}, 5000), 1000, nil)
	if _, err := DecompressAny(big, 4999); !errors.Is(err, ErrTooLarge) {
		t.Errorf("maxOutput one short: %v; want ErrTooLarge", err)
	}
	if got, err := DecompressAny(big, 5000); err != nil || len(got) != 5000 {
		t.Errorf("maxOutput exact: %d, %v", len(got), err)
	}
}
