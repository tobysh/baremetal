package main

import (
	"bytes"
	"errors"
	"hash/crc32"
	"math/rand"
	"testing"
)

func refFrame(typ byte, p []byte) []byte {
	b := []byte{0xBE, typ, byte(len(p) >> 8), byte(len(p))}
	b = append(b, p...)
	s := crc32.ChecksumIEEE(b)
	return append(b, byte(s>>24), byte(s>>16), byte(s>>8), byte(s))
}

type refMsg struct {
	typ byte
	p   []byte
}

func refStream(seed int64, count int) ([]byte, []refMsg) {
	r := rand.New(rand.NewSource(seed))
	var stream []byte
	var msgs []refMsg
	for i := 0; i < count; i++ {
		n := []int{0, 1, 5, 40, 300, 1024}[r.Intn(6)]
		p := make([]byte, n)
		r.Read(p)
		typ := byte(r.Intn(250))
		msgs = append(msgs, refMsg{typ, p})
		stream = append(stream, refFrame(typ, p)...)
	}
	return stream, msgs
}

func check(t *testing.T, label string, got []Frame, want []refMsg) {
	t.Helper()
	if len(got) != len(want) {
		t.Fatalf("%s: got %d frames; want %d", label, len(got), len(want))
	}
	for i := range want {
		if got[i].Type != want[i].typ || !bytes.Equal(got[i].Payload, want[i].p) {
			t.Fatalf("%s: frame %d differs (type %d len %d; want type %d len %d)", label, i, got[i].Type, len(got[i].Payload), want[i].typ, len(want[i].p))
		}
	}
}

// feedAll feeds stream in chunks of the given sizes through a shared buffer
// that is scribbled over after each call.
func feedAll(t *testing.T, stream []byte, sizes func() int) ([]Frame, *Parser) {
	t.Helper()
	p := NewParser()
	var out []Frame
	shared := make([]byte, 0, 4096)
	for pos := 0; pos < len(stream); {
		n := sizes()
		if n > len(stream)-pos {
			n = len(stream) - pos
		}
		shared = append(shared[:0], stream[pos:pos+n]...)
		frames, err := p.Feed(shared)
		for i := range shared {
			shared[i] = 0xEE
		}
		if err != nil {
			t.Fatalf("unexpected error at offset %d: %v", pos, err)
		}
		if p.Buffered() >= HeaderLen+MaxPayload+TrailerLen {
			t.Fatalf("Buffered = %d; a complete frame must never stay buffered", p.Buffered())
		}
		out = append(out, frames...)
		pos += n
	}
	return out, p
}

func TestWhole_stream_in_one_chunk(t *testing.T) {
	stream, msgs := refStream(1, 25)
	got, p := feedAll(t, stream, func() int { return len(stream) })
	check(t, "whole", got, msgs)
	if p.Buffered() != 0 {
		t.Errorf("Buffered = %d after a complete stream; want 0", p.Buffered())
	}
}

func TestOne_byte_at_a_time(t *testing.T) {
	stream, msgs := refStream(2, 12)
	got, _ := feedAll(t, stream, func() int { return 1 })
	check(t, "bytewise", got, msgs)
}

func TestEvery_two_way_split(t *testing.T) {
	stream, msgs := refStream(3, 3)
	for cut := 0; cut <= len(stream); cut++ {
		p := NewParser()
		var got []Frame
		a := append([]byte(nil), stream[:cut]...)
		b := append([]byte(nil), stream[cut:]...)
		f1, err1 := p.Feed(a)
		f2, err2 := p.Feed(b)
		if err1 != nil || err2 != nil {
			t.Fatalf("cut %d: errors %v %v", cut, err1, err2)
		}
		got = append(append(got, f1...), f2...)
		check(t, "split", got, msgs)
	}
}

func TestRandom_chunkings(t *testing.T) {
	for seed := int64(0); seed < 150; seed++ {
		stream, msgs := refStream(100+seed, 1+int(seed%9))
		r := rand.New(rand.NewSource(seed))
		max := []int{2, 7, 64, 1500}[seed%4]
		got, p := feedAll(t, stream, func() int { return r.Intn(max + 1) }) // includes empty chunks
		check(t, "random", got, msgs)
		if p.Buffered() != 0 {
			t.Fatalf("seed %d: %d bytes left over", seed, p.Buffered())
		}
	}
}

func TestPartial_frame_stays_buffered(t *testing.T) {
	f := refFrame(5, []byte("0123456789"))
	p := NewParser()
	frames, err := p.Feed(f[:len(f)-1])
	if len(frames) != 0 || err != nil || p.Buffered() != len(f)-1 {
		t.Fatalf("partial: %d frames, err %v, buffered %d", len(frames), err, p.Buffered())
	}
	frames, _ = p.Feed(f[len(f)-1:])
	if len(frames) != 1 || p.Buffered() != 0 {
		t.Fatalf("completion: %d frames, buffered %d", len(frames), p.Buffered())
	}
}

func TestFrames_do_not_alias_internal_buffer(t *testing.T) {
	a := refFrame(1, []byte("first-payload"))
	b := refFrame(2, []byte("second-payload"))
	p := NewParser()
	f1, _ := p.Feed(append(append([]byte(nil), a...), b[:3]...))
	p.Feed(b[3:])
	p.Feed(refFrame(3, bytes.Repeat([]byte{'Z'}, 400)))
	if len(f1) != 1 || string(f1[0].Payload) != "first-payload" {
		t.Errorf("an earlier frame was lost or its payload changed (%d frames)", len(f1))
	}
}

func TestBad_checksum_latches(t *testing.T) {
	good := refFrame(1, []byte("ok"))
	bad := refFrame(2, []byte("damaged"))
	bad[5] ^= 0x40
	p := NewParser()
	frames, err := p.Feed(append(append(append([]byte(nil), good...), bad...), good...))
	if !errors.Is(err, ErrChecksum) || len(frames) != 1 || frames[0].Type != 1 {
		t.Fatalf("got %d frames, err %v; want the 1 frame before the damage and ErrChecksum", len(frames), err)
	}
	frames, err2 := p.Feed(good)
	if len(frames) != 0 || !errors.Is(err2, ErrChecksum) {
		t.Errorf("after the error: %d frames, %v; want none and the same error", len(frames), err2)
	}
	if p.Buffered() != 0 {
		t.Errorf("Buffered = %d after an error; want 0", p.Buffered())
	}
}

func TestOversize_header_rejected_without_waiting(t *testing.T) {
	p := NewParser()
	_, err := p.Feed([]byte{0xBE, 1, 0xFF, 0xFF})
	if !errors.Is(err, ErrTooLarge) {
		t.Errorf("oversize header: %v; want ErrTooLarge immediately", err)
	}
	q := NewParser()
	if _, err := q.Feed([]byte{0x00}); !errors.Is(err, ErrMagic) {
		t.Errorf("bad magic: %v; want ErrMagic", err)
	}
}
