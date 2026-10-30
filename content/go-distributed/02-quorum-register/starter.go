package main

import (
	"errors"
	"fmt"
)

// Versioned is a value with its version. Version 0 is "never written".
type Versioned struct {
	Ver int
	Val string
}

// Peer is a replica reached over a network. Either call may fail, and a
// failed Put may or may not have been applied.
type Peer interface {
	Get() (Versioned, error)
	Put(v Versioned) error
}

// Replica is a local, always-reachable replica: it keeps the highest version
// it has been given. (Supplied. The hidden tests wrap it with failures.)
type Replica struct {
	cur Versioned
}

func (r *Replica) Get() (Versioned, error) { return r.cur, nil }

func (r *Replica) Put(v Versioned) error {
	if v.Ver > r.cur.Ver {
		r.cur = v
	}
	return nil
}

var ErrNoQuorum = errors.New("no quorum")

type Register struct {
	peers []Peer
	// add your state here
}

func NewRegister(peers []Peer) *Register {
	return &Register{peers: peers}
}

func (r *Register) Write(val string) error {
	return ErrNoQuorum
}

func (r *Register) Read() (string, error) {
	return "", ErrNoQuorum
}

// Try it: three healthy replicas.
func main() {
	peers := []Peer{&Replica{}, &Replica{}, &Replica{}}
	reg := NewRegister(peers)
	fmt.Println(reg.Write("hello"))
	fmt.Println(reg.Read())
}
