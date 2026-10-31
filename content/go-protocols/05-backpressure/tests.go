package main

import (
	"bytes"
	"errors"
	"math/rand"
	"testing"
)

func TestSend_cap_is_in_bytes_and_inclusive(t *testing.T) {
	// A frame with a 4 byte payload is 12 bytes on the wire.
	s := NewSender(36, 10)
	for i := 0; i < 3; i++ {
		if err := s.Send(1, make([]byte, 4)); err != nil {
			t.Fatalf("send %d: %v (36 bytes fits exactly three 12 byte frames)", i, err)
		}
	}
	if err := s.Send(1, nil); !errors.Is(err, ErrQueueFull) {
		t.Errorf("fourth send: %v; want ErrQueueFull", err)
	}
	if s.QueuedBytes() != 36 || s.QueuedFrames() != 3 {
		t.Errorf("a refused Send changed state: %d bytes %d frames", s.QueuedBytes(), s.QueuedFrames())
	}
	if _, ok := s.Next(); !ok {
		t.Fatal("no frame released")
	}
	if err := s.Send(1, make([]byte, 4)); err != nil {
		t.Errorf("after Next freed 12 bytes: %v", err)
	}
}

func TestSend_errors_and_copy(t *testing.T) {
	s := NewSender(10000, 1)
	if err := s.Send(1, make([]byte, MaxPayload+1)); !errors.Is(err, ErrTooLarge) {
		t.Errorf("oversize: %v", err)
	}
	if err := NewSender(5, 1).Send(1, nil); !errors.Is(err, ErrQueueFull) {
		t.Errorf("a frame larger than the whole cap: %v; want ErrQueueFull", err)
	}
	buf := []byte("abc")
	s.Send(7, buf)
	copy(buf, "XYZ")
	b, _ := s.Next()
	_, p, _, err := Decode(b)
	if err != nil || string(p) != "abc" {
		t.Errorf("queued payload %q, %v; the sender must copy at Send time", p, err)
	}
}

func TestCredit_window_and_ack(t *testing.T) {
	s := NewSender(1<<20, 2)
	for i := 0; i < 5; i++ {
		s.Send(1, []byte{byte(i)})
	}
	var order []byte
	for {
		b, ok := s.Next()
		if !ok {
			break
		}
		_, p, _, _ := Decode(b)
		order = append(order, p[0])
	}
	if !bytes.Equal(order, []byte{0, 1}) || s.InFlight() != 2 || s.QueuedFrames() != 3 {
		t.Fatalf("released %v with %d in flight; want [0 1], window of 2", order, s.InFlight())
	}
	if err := s.Ack(3); !errors.Is(err, ErrBadAck) || s.InFlight() != 2 {
		t.Errorf("Ack(3) with 2 in flight: %v, in flight %d", err, s.InFlight())
	}
	if err := s.Ack(-1); !errors.Is(err, ErrBadAck) {
		t.Errorf("Ack(-1): %v", err)
	}
	if err := s.Ack(1); err != nil {
		t.Fatal(err)
	}
	b, ok := s.Next()
	_, p, _, _ := Decode(b)
	if !ok || p[0] != 2 {
		t.Errorf("after one ack got %v %v; want frame 2", p, ok)
	}
	if _, ok := s.Next(); ok {
		t.Errorf("released a frame with the window full")
	}
	if err := s.Ack(0); err != nil {
		t.Errorf("Ack(0): %v", err)
	}
}

// model is a deliberately naive reference.
type model struct {
	sizes    []int
	ids      []int
	inflight int
	max, win int
}

func (m *model) bytes() (n int) {
	for _, s := range m.sizes {
		n += s
	}
	return
}

func TestRandom_operations_against_model(t *testing.T) {
	for seed := int64(0); seed < 40; seed++ {
		r := rand.New(rand.NewSource(seed))
		max := 30 + r.Intn(300)
		win := 1 + r.Intn(5)
		s := NewSender(max, win)
		m := &model{max: max, win: win}
		id := 0
		for step := 0; step < 1500; step++ {
			switch op := r.Intn(10); {
			case op < 5:
				n := r.Intn(50)
				id++
				err := s.Send(1, append([]byte{byte(id), byte(id >> 8)}, make([]byte, n)...))
				size := 8 + 2 + n
				if m.bytes()+size > m.max {
					if !errors.Is(err, ErrQueueFull) {
						t.Fatalf("seed %d step %d: Send = %v; want ErrQueueFull (queued %d + %d > %d)", seed, step, err, m.bytes(), size, max)
					}
				} else {
					if err != nil {
						t.Fatalf("seed %d step %d: Send = %v; want success", seed, step, err)
					}
					m.sizes = append(m.sizes, size)
					m.ids = append(m.ids, id)
				}
			case op < 8:
				b, ok := s.Next()
				wantOK := len(m.ids) > 0 && m.inflight < m.win
				if ok != wantOK {
					t.Fatalf("seed %d step %d: Next ok=%v; want %v", seed, step, ok, wantOK)
				}
				if ok {
					_, p, _, err := Decode(b)
					got := int(p[0]) | int(p[1])<<8
					if err != nil || got != m.ids[0]&0xffff {
						t.Fatalf("seed %d step %d: released frame %d (%v); want %d", seed, step, got, err, m.ids[0])
					}
					m.sizes, m.ids, m.inflight = m.sizes[1:], m.ids[1:], m.inflight+1
				}
			default:
				n := r.Intn(m.win+2) - 1
				err := s.Ack(n)
				if n < 0 || n > m.inflight {
					if !errors.Is(err, ErrBadAck) {
						t.Fatalf("seed %d step %d: Ack(%d) = %v; want ErrBadAck", seed, step, n, err)
					}
				} else if err != nil {
					t.Fatalf("seed %d step %d: Ack(%d) = %v", seed, step, n, err)
				} else {
					m.inflight -= n
				}
			}
			if s.InFlight() != m.inflight || s.QueuedFrames() != len(m.ids) || s.QueuedBytes() != m.bytes() {
				t.Fatalf("seed %d step %d: state %d/%d/%d; want %d/%d/%d (inflight/frames/bytes)", seed, step,
					s.InFlight(), s.QueuedFrames(), s.QueuedBytes(), m.inflight, len(m.ids), m.bytes())
			}
		}
	}
}

func TestSlow_consumer_simulation(t *testing.T) {
	const total, maxQ, win = 300, 100, 3
	s := NewSender(maxQ, win)
	next := 0
	var wire [][]byte // frames the "network" holds
	var delivered []int
	maxInflight, maxQueued, refusals := 0, 0, 0
	for tick := 0; len(delivered) < total; tick++ {
		if tick > 100000 {
			t.Fatal("simulation did not finish")
		}
		// producer: tries to enqueue 3 messages per tick, backs off when refused
		for k := 0; k < 3 && next < total; k++ {
			err := s.Send(1, []byte{byte(next), byte(next >> 8), 0, 0, 0, 0})
			if errors.Is(err, ErrQueueFull) {
				refusals++
				break
			} else if err != nil {
				t.Fatal(err)
			}
			next++
		}
		for {
			b, ok := s.Next()
			if !ok {
				break
			}
			wire = append(wire, b)
		}
		// consumer: takes one frame every other tick and acks it
		if tick%2 == 0 && len(wire) > 0 {
			_, p, _, _ := Decode(wire[0])
			wire = wire[1:]
			delivered = append(delivered, int(p[0])|int(p[1])<<8)
			if err := s.Ack(1); err != nil {
				t.Fatal(err)
			}
		}
		if s.InFlight() > maxInflight {
			maxInflight = s.InFlight()
		}
		if s.QueuedBytes() > maxQueued {
			maxQueued = s.QueuedBytes()
		}
	}
	for i, d := range delivered {
		if d != i {
			t.Fatalf("delivery %d was message %d: lost, duplicated or reordered", i, d)
		}
	}
	if maxInflight > win || maxQueued > maxQ {
		t.Errorf("limits exceeded: in flight %d (window %d), queued %d bytes (cap %d)", maxInflight, win, maxQueued, maxQ)
	}
	if refusals == 0 {
		t.Errorf("the producer was never refused; the queue is not applying backpressure")
	}
}
