package main

import (
	"encoding/binary"
	"errors"
	"fmt"
)

// ---- given ----

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

func EncodeAgreement(a Agreement) []byte {
	return binary.BigEndian.AppendUint32([]byte{a.Version}, a.Features)
}

func DecodeAgreement(p []byte) (Agreement, error) {
	if len(p) < 5 {
		return Agreement{}, ErrBadHello
	}
	return Agreement{p[0], binary.BigEndian.Uint32(p[1:])}, nil
}

// ---- your work ----

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

// Try it with Run.
func main() {
	newer := Hello{MinVer: 1, MaxVer: 3, Features: FeatAck | FeatCompress | FeatBatch | 1<<9}
	older := Hello{MinVer: 1, MaxVer: 2, Features: FeatAck | FeatCompress}
	fmt.Println(Negotiate(newer, older))
	strict := Hello{MinVer: 3, MaxVer: 3, Features: FeatBatch, Required: FeatBatch}
	fmt.Println(Negotiate(strict, older))
	h, err := DecodeHello(EncodeHello(newer))
	fmt.Println(h == newer, err)
}
