package main

import (
	"errors"
	"fmt"
)

const Skew int64 = 300

var (
	ErrMalformed        = errors.New("malformed offer")
	ErrFuture           = errors.New("offer issued in the future")
	ErrExpired          = errors.New("offer expired")
	ErrStaleMetadata    = errors.New("offer older than metadata already seen")
	ErrAlreadyInstalled = errors.New("already installed")
	ErrEquivocation     = errors.New("same counter, different content")
	ErrDowngrade        = errors.New("downgrade refused")
)

type State struct {
	Counter, HighWater uint64
	Digest             string
	LastIssued         int64
}

type Offer struct {
	Version              string
	Counter              uint64
	IssuedAt, ExpiresAt int64
	Digest               string
}

func Decide(st State, o Offer, now int64) error {
	return nil
}

func Apply(st State, o Offer) State {
	return st
}

// Try it with Run.
func main() {
	st := State{}
	history := []struct {
		name string
		o    Offer
		now  int64
	}{
		{"v1", Offer{"1.0", 1, 0, 1000, "d1"}, 10},
		{"v2", Offer{"1.1", 2, 100, 1000, "d2"}, 120},
		{"replay v1", Offer{"1.0", 1, 0, 1000, "d1"}, 130},
		{"replay v2", Offer{"1.1", 2, 100, 1000, "d2"}, 140},
		{"v3 expired", Offer{"1.2", 3, 150, 200, "d3"}, 250},
		{"v3", Offer{"1.2", 3, 150, 2000, "d3"}, 250},
	}
	for _, h := range history {
		err := Decide(st, h.o, h.now)
		fmt.Printf("%-11s -> %v\n", h.name, err)
		if err == nil {
			st = Apply(st, h.o)
		}
	}
}
