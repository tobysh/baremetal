package main

import (
	"errors"
	"fmt"
)

type Versioned struct {
	Ver int
	Val string
}

type Peer interface {
	Get() (Versioned, error)
	Put(v Versioned) error
}

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
	last  int
}

func NewRegister(peers []Peer) *Register {
	return &Register{peers: peers}
}

func (r *Register) quorum() int { return len(r.peers)/2 + 1 }

func (r *Register) collect() (Versioned, bool) {
	var best Versioned
	ok := 0
	for _, p := range r.peers {
		v, err := p.Get()
		if err != nil {
			continue
		}
		ok++
		if v.Ver > best.Ver {
			best = v
		}
	}
	return best, ok >= r.quorum()
}

func (r *Register) putAll(v Versioned) bool {
	acks := 0
	for _, p := range r.peers {
		if p.Put(v) == nil {
			acks++
		}
	}
	return acks >= r.quorum()
}

func (r *Register) Write(val string) error {
	best, ok := r.collect()
	if !ok {
		return ErrNoQuorum
	}
	ver := best.Ver
	if r.last > ver {
		ver = r.last
	}
	ver++
	r.last = ver
	if !r.putAll(Versioned{ver, val}) {
		return ErrNoQuorum
	}
	return nil
}

func (r *Register) Read() (string, error) {
	best, ok := r.collect()
	if !ok {
		return "", ErrNoQuorum
	}
	if best.Ver > 0 && !r.putAll(best) {
		return "", ErrNoQuorum
	}
	return best.Val, nil
}

func main() {
	peers := []Peer{&Replica{}, &Replica{}, &Replica{}}
	reg := NewRegister(peers)
	fmt.Println(reg.Write("hello"))
	fmt.Println(reg.Read())
}
