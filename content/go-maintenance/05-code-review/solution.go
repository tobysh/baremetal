package main

import (
	"errors"
	"fmt"
	"sort"
)

// Cap is the largest balance any name may hold.
const Cap = 100

// Op is one step of a scenario.
type Op struct {
	Kind string // add, use, refund, transfer, scribble
	Name string
	To   string // transfer destination
	N    int
}

// Pool tracks credit balances. Regions are marked [A]..[H].
type Pool struct {
	bal   map[string]int
	hist  map[string][]string
	names []string // in order of first credit
}

func NewPool() *Pool {
	return &Pool{bal: map[string]int{}, hist: map[string][]string{}}
}

func (p *Pool) touch(name string) {
	if _, ok := p.bal[name]; !ok {
		p.bal[name] = 0
		p.names = append(p.names, name)
	}
}

// [A] Add credits a name.
func (p *Pool) Add(name string, n int) error {
	if n <= 0 {
		return errors.New("bad amount")
	}
	// [H] cap check
	if p.bal[name]+n > Cap {
		return errors.New("over cap")
	}
	p.touch(name)
	p.bal[name] += n
	p.hist[name] = append(p.hist[name], fmt.Sprintf("add %d", n))
	return nil
}

// [B] Use spends credit.
func (p *Pool) Use(name string, n int) error {
	if n < 0 {
		return errors.New("bad amount")
	}
	// [E] zero is a no-op
	if n == 0 {
		return nil
	}
	if p.bal[name] < n {
		return errors.New("insufficient")
	}
	p.bal[name] -= n
	p.hist[name] = append(p.hist[name], fmt.Sprintf("use %d", n))
	return nil
}

// [C] Refund gives credit back.
func (p *Pool) Refund(name string, n int) error {
	if n <= 0 {
		return errors.New("bad amount")
	}
	p.touch(name)
	p.bal[name] += n
	if p.bal[name] > Cap {
		p.bal[name] = Cap
	}
	p.hist[name] = append(p.hist[name], fmt.Sprintf("refund %d", n))
	return nil
}

// [D] Transfer moves credit between names.
func (p *Pool) Transfer(from, to string, n int) error {
	if n <= 0 {
		return errors.New("bad amount")
	}
	if p.bal[from] < n {
		return errors.New("insufficient")
	}
	if p.bal[to]+n > Cap {
		return errors.New("over cap")
	}
	p.bal[from] -= n
	p.touch(to)
	p.bal[to] += n
	p.hist[from] = append(p.hist[from], fmt.Sprintf("out %d", n))
	p.hist[to] = append(p.hist[to], fmt.Sprintf("in %d", n))
	return nil
}

// [F] History returns the entries recorded for a name.
func (p *Pool) History(name string) []string {
	return append([]string(nil), p.hist[name]...)
}

// [G] Report lists balances.
func (p *Pool) Report() string {
	names := append([]string(nil), p.names...)
	sort.Strings(names)
	out := ""
	for i, n := range names {
		if i > 0 {
			out += ","
		}
		out += fmt.Sprintf("%s=%d", n, p.bal[n])
	}
	return out
}

// Found returns the ids of regions with a real defect.
func Found() []string { return []string{"A", "B", "C", "D", "F", "G"} }

// Counterexample returns operations that expose the defect in region id.
func Counterexample(id string) []Op {
	switch id {
	case "A":
		return []Op{{Kind: "add", Name: "ann", N: 0}}
	case "B":
		return []Op{{Kind: "add", Name: "ann", N: 5}, {Kind: "use", Name: "ann", N: 5}}
	case "C":
		return []Op{{Kind: "add", Name: "ann", N: 90}, {Kind: "refund", Name: "ann", N: 20}}
	case "D":
		return []Op{{Kind: "add", Name: "ann", N: 10}, {Kind: "add", Name: "bob", N: 95}, {Kind: "transfer", Name: "ann", To: "bob", N: 10}}
	case "F":
		return []Op{{Kind: "add", Name: "ann", N: 5}, {Kind: "scribble", Name: "ann"}}
	case "G":
		return []Op{{Kind: "add", Name: "bob", N: 1}, {Kind: "add", Name: "ann", N: 2}}
	}
	return nil
}

func main() {
	p := NewPool()
	p.Add("ann", 90)
	p.Refund("ann", 20)
	p.Add("bob", 5)
	fmt.Println(p.Report(), p.History("ann"))
}
