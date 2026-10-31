package main

import (
	"bytes"
	"encoding/binary"
	"encoding/hex"
	"errors"
	"hash/crc32"
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
