package main

import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"
	"hash/crc32"
)

// ---- given: earlier lessons ----

const (
	Magic      = 0xBE
	HeaderLen  = 4
	TrailerLen = 4
	MaxPayload = 1024
)

var (
	ErrShort    = errors.New("frame incomplete: need more bytes")
	ErrMagic    = errors.New("bad magic byte")
	ErrTooLarge = errors.New("payload too large")
	ErrChecksum = errors.New("checksum mismatch")
)

type Frame struct {
	Type    byte
	Payload []byte
}

// Encode builds [Magic][type][len hi][len lo][payload][crc32 BE of everything before it].
func Encode(typ byte, payload []byte) ([]byte, error) {
	if len(payload) > MaxPayload {
		return nil, ErrTooLarge
	}
	b := make([]byte, 0, HeaderLen+len(payload)+TrailerLen)
	b = append(b, Magic, typ, byte(len(payload)>>8), byte(len(payload)))
	b = append(b, payload...)
	s := crc32.ChecksumIEEE(b)
	return append(b, byte(s>>24), byte(s>>16), byte(s>>8), byte(s)), nil
}

// Decode reads one frame from the start of b. It returns ErrShort while more
// bytes are needed, and never reads past what the header promises.
func Decode(b []byte) (typ byte, payload []byte, n int, err error) {
	if len(b) == 0 {
		return 0, nil, 0, ErrShort
	}
	if b[0] != Magic {
		return 0, nil, 0, ErrMagic
	}
	if len(b) < HeaderLen {
		return 0, nil, 0, ErrShort
	}
	l := int(b[2])<<8 | int(b[3])
	if l > MaxPayload {
		return 0, nil, 0, ErrTooLarge
	}
	total := HeaderLen + l + TrailerLen
	if len(b) < total {
		return 0, nil, 0, ErrShort
	}
	want := uint32(b[total-4])<<24 | uint32(b[total-3])<<16 | uint32(b[total-2])<<8 | uint32(b[total-1])
	if crc32.ChecksumIEEE(b[:total-4]) != want {
		return 0, nil, 0, ErrChecksum
	}
	p := make([]byte, l)
	copy(p, b[HeaderLen:HeaderLen+l])
	return b[1], p, total, nil
}

var ErrTruncated = errors.New("stream ended inside a frame")

type Scanner struct {
	buf      []byte
	skipping bool
	dropped  int
}

func NewScanner() *Scanner { return &Scanner{} }

func (s *Scanner) Feed(chunk []byte) ([]Frame, []error) {
	s.buf = append(s.buf, chunk...)
	return s.run(false)
}

func (s *Scanner) Close() ([]Frame, []error) { return s.run(true) }

func (s *Scanner) Buffered() int { return len(s.buf) }
func (s *Scanner) Dropped() int  { return s.dropped }

func (s *Scanner) run(final bool) (frames []Frame, errs []error) {
	for len(s.buf) > 0 {
		typ, payload, n, err := Decode(s.buf)
		if err == nil {
			frames = append(frames, Frame{typ, payload})
			s.buf = s.buf[n:]
			s.skipping = false
			continue
		}
		if err == ErrShort {
			if !final {
				break
			}
			err = ErrTruncated
		}
		if !s.skipping {
			errs = append(errs, err)
			s.skipping = true
		}
		i := bytes.IndexByte(s.buf[1:], Magic)
		if i < 0 {
			s.dropped += len(s.buf)
			s.buf = nil
		} else {
			s.dropped += i + 1
			s.buf = s.buf[i+1:]
		}
	}
	if len(s.buf) == 0 {
		s.buf = nil
	}
	return
}

const (
	FeatAck      uint32 = 1 << 0
	FeatCompress uint32 = 1 << 1
	FeatBatch    uint32 = 1 << 2
)

// featureSince is the first protocol version in which a feature exists.
// Bits not in this table are unknown to us.
var featureSince = map[uint32]uint8{FeatAck: 1, FeatCompress: 2, FeatBatch: 3}

var (
	ErrBadHello       = errors.New("malformed hello")
	ErrNoVersion      = errors.New("no common protocol version")
	ErrMissingFeature = errors.New("required feature not available")
)

type Hello struct {
	MinVer, MaxVer     uint8
	Features, Required uint32
}

type Agreement struct {
	Version  uint8
	Features uint32
}

func EncodeHello(h Hello) []byte {
	b := []byte{h.MinVer, h.MaxVer}
	b = binary.BigEndian.AppendUint32(b, h.Features)
	return binary.BigEndian.AppendUint32(b, h.Required)
}

// DecodeHello accepts the 6-byte v1 form (no Required field), the 10-byte
// form, and ignores any bytes after those (future extensions).
func DecodeHello(p []byte) (Hello, error) {
	if len(p) < 6 {
		return Hello{}, ErrBadHello
	}
	h := Hello{MinVer: p[0], MaxVer: p[1], Features: binary.BigEndian.Uint32(p[2:])}
	if len(p) >= 10 {
		h.Required = binary.BigEndian.Uint32(p[6:])
	}
	return h, nil
}

func Negotiate(local, remote Hello) (Agreement, error) {
	for _, h := range []Hello{local, remote} {
		if h.MinVer == 0 || h.MinVer > h.MaxVer {
			return Agreement{}, ErrBadHello
		}
	}
	v := local.MaxVer
	if remote.MaxVer < v {
		v = remote.MaxVer
	}
	lo := local.MinVer
	if remote.MinVer > lo {
		lo = remote.MinVer
	}
	if v < lo {
		return Agreement{}, ErrNoVersion
	}
	var feats uint32
	for bit, since := range featureSince {
		if local.Features&bit != 0 && remote.Features&bit != 0 && since <= v {
			feats |= bit
		}
	}
	if (local.Required|remote.Required)&^feats != 0 {
		return Agreement{}, ErrMissingFeature
	}
	return Agreement{v, feats}, nil
}

func EncodeAgreement(a Agreement) []byte {
	return binary.BigEndian.AppendUint32([]byte{a.Version}, a.Features)
}

func DecodeAgreement(p []byte) (Agreement, error) {
	if len(p) < 5 {
		return Agreement{}, ErrBadHello
	}
	return Agreement{p[0], binary.BigEndian.Uint32(p[1:])}, nil
}

// Message types.
const (
	TypeHello   = 1 // client -> server: EncodeHello payload
	TypeWelcome = 2 // server -> client: EncodeAgreement payload
	TypeReq     = 3 // client -> server: seq (u32 BE) + body
	TypeResp    = 4 // server -> client: seq (u32 BE) + body reversed
	TypeErr     = 5 // server -> client: text
	TypeBye     = 6 // client -> server: no payload
)

const (
	Window     = 3  // max distinct unanswered requests in flight
	RetryTicks = 4  // retransmit after this many fake-clock ticks of silence
	MaxRetries = 30 // retransmissions of one message before giving up
)

var (
	ErrGiveUp   = errors.New("peer unresponsive: retries exhausted")
	ErrRejected = errors.New("server rejected the handshake")
)

func mustEncode(typ byte, payload []byte) []byte {
	b, err := Encode(typ, payload)
	if err != nil {
		panic(err) // only for payloads known to fit
	}
	return b
}

// ---- your work ----

type Server struct{}

func NewServer(h Hello) *Server          { return &Server{} }
func (s *Server) OnData(p []byte) []byte { return nil }
func (s *Server) Closed() bool           { return false }

type Client struct{}

func NewClient(h Hello, reqs [][]byte) *Client    { return &Client{} }
func (c *Client) Tick(now int) []byte             { return nil }
func (c *Client) OnData(now int, p []byte) []byte { return nil }
func (c *Client) Done() bool                      { return false }
func (c *Client) Err() error                      { return nil }
func (c *Client) Results() [][]byte               { return nil }
func (c *Client) Version() uint8                  { return 0 }
func (c *Client) Features() uint32                { return 0 }

var _ = []any{bytes.IndexByte, binary.BigEndian, fmt.Sprint} // keep imports while you work

// Try it with Run: a perfect link with no faults.
func main() {
	srv := NewServer(Hello{MinVer: 1, MaxVer: 3, Features: FeatAck | FeatCompress})
	cli := NewClient(Hello{MinVer: 1, MaxVer: 2, Features: FeatAck}, [][]byte{[]byte("abc"), []byte("hello")})
	for now := 0; now < 50 && !cli.Done(); now++ {
		out := cli.Tick(now)
		for len(out) > 0 {
			reply := srv.OnData(out)
			out = cli.OnData(now, reply)
		}
	}
	for _, r := range cli.Results() {
		fmt.Printf("%s\n", r)
	}
	fmt.Println("version", cli.Version(), "err", cli.Err(), "done", cli.Done())
}
