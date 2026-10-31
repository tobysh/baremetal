package main

import (
	"errors"
	"fmt"
	"sort"
	"strings"
	"testing"
)

// poolAPI is what the harness needs from a pool.
type poolAPI interface {
	Add(string, int) error
	Use(string, int) error
	Refund(string, int) error
	Transfer(string, string, int) error
	History(string) []string
	Report() string
}

// refPool is the specification. Each flag switches on exactly one planted defect.
type refPool struct {
	bugs  map[string]bool
	bal   map[string]int
	hist  map[string][]string
	order []string
}

func newRef(bugs ...string) *refPool {
	r := &refPool{bugs: map[string]bool{}, bal: map[string]int{}, hist: map[string][]string{}}
	for _, b := range bugs {
		r.bugs[b] = true
	}
	return r
}

func (r *refPool) credit(name string, n int) {
	if _, ok := r.bal[name]; !ok {
		r.order = append(r.order, name)
	}
	r.bal[name] += n
}

func (r *refPool) Add(name string, n int) error {
	if n < 0 || (n == 0 && !r.bugs["A"]) {
		return errors.New("bad")
	}
	if r.bal[name]+n > 100 {
		return errors.New("cap")
	}
	r.credit(name, n)
	r.hist[name] = append(r.hist[name], fmt.Sprint("add ", n))
	return nil
}

func (r *refPool) Use(name string, n int) error {
	if n < 0 {
		return errors.New("bad")
	}
	if n == 0 {
		return nil
	}
	if r.bal[name] < n || (r.bugs["B"] && r.bal[name] == n) {
		return errors.New("insufficient")
	}
	r.bal[name] -= n
	r.hist[name] = append(r.hist[name], fmt.Sprint("use ", n))
	return nil
}

func (r *refPool) Refund(name string, n int) error {
	if n <= 0 {
		return errors.New("bad")
	}
	r.credit(name, n)
	if r.bal[name] > 100 && !r.bugs["C"] {
		r.bal[name] = 100
	}
	r.hist[name] = append(r.hist[name], fmt.Sprint("refund ", n))
	return nil
}

func (r *refPool) Transfer(from, to string, n int) error {
	if n <= 0 || r.bal[from] < n {
		return errors.New("bad")
	}
	if r.bugs["D"] {
		r.bal[from] -= n
	}
	if r.bal[to]+n > 100 {
		return errors.New("cap")
	}
	if !r.bugs["D"] {
		r.bal[from] -= n
	}
	r.credit(to, n)
	r.hist[from] = append(r.hist[from], fmt.Sprint("out ", n))
	r.hist[to] = append(r.hist[to], fmt.Sprint("in ", n))
	return nil
}

func (r *refPool) History(name string) []string {
	if r.bugs["F"] {
		return r.hist[name]
	}
	return append([]string(nil), r.hist[name]...)
}

func (r *refPool) Report() string {
	names := append([]string(nil), r.order...)
	if !r.bugs["G"] {
		sort.Strings(names)
	}
	parts := make([]string, len(names))
	for i, n := range names {
		parts[i] = fmt.Sprintf("%s=%d", n, r.bal[n])
	}
	return strings.Join(parts, ",")
}

var allNames = []string{"ann", "bob", "cy"}

// play runs ops and returns everything observable.
func play(p poolAPI, ops []Op) string {
	var out []string
	res := func(err error) {
		if err != nil {
			out = append(out, "err")
		} else {
			out = append(out, "ok")
		}
	}
	for _, o := range ops {
		switch o.Kind {
		case "add":
			res(p.Add(o.Name, o.N))
		case "use":
			res(p.Use(o.Name, o.N))
		case "refund":
			res(p.Refund(o.Name, o.N))
		case "transfer":
			res(p.Transfer(o.Name, o.To, o.N))
		case "scribble":
			if h := p.History(o.Name); len(h) > 0 {
				h[0] = "X"
			}
			out = append(out, "scribbled")
		}
	}
	out = append(out, p.Report())
	for _, n := range allNames {
		out = append(out, n+":"+strings.Join(p.History(n), ";"))
	}
	return strings.Join(out, "|")
}

type rng uint64

func (r *rng) next(n int) int {
	*r = *r*6364136223846793005 + 1442695040888963407
	return int((uint64(*r) >> 33) % uint64(n))
}

func genOps(seed uint64, length int) []Op {
	r := rng(seed*7919 + 13)
	kinds := []string{"add", "add", "add", "use", "use", "refund", "transfer", "transfer", "scribble"}
	amounts := []int{0, 0, 1, 2, 5, 10, 30, 50, 60, 90, 99, 100, 101, -1}
	ops := make([]Op, length)
	for i := range ops {
		ops[i] = Op{
			Kind: kinds[r.next(len(kinds))],
			Name: allNames[r.next(3)],
			To:   allNames[r.next(3)],
			N:    amounts[r.next(len(amounts))],
		}
	}
	return ops
}

var planted = []string{"A", "B", "C", "D", "F", "G"}

func TestHarness_random_sequences_expose_each_planted_defect(t *testing.T) {
	for _, b := range planted {
		found := false
		for seed := uint64(0); seed < 400 && !found; seed++ {
			ops := genOps(seed, 25)
			found = play(newRef(), ops) != play(newRef(b), ops)
		}
		if !found {
			t.Fatalf("harness bug: generated sequences never expose %s", b)
		}
	}
}

func TestFound_matches_the_inventory(t *testing.T) {
	got := append([]string(nil), Found()...)
	sort.Strings(got)
	want := planted
	if strings.Join(got, ",") != strings.Join(want, ",") {
		missing, extra := diff(want, got)
		t.Errorf("Found() = %v. Missing real defects: %v. Reported regions that are correct: %v", got, missing, extra)
	}
}

func diff(want, got []string) (missing, extra []string) {
	in := func(s []string, x string) bool {
		for _, v := range s {
			if v == x {
				return true
			}
		}
		return false
	}
	for _, w := range want {
		if !in(got, w) {
			missing = append(missing, w)
		}
	}
	for _, g := range got {
		if !in(want, g) {
			extra = append(extra, g)
		}
	}
	return
}

func TestCounterexamples_expose_exactly_their_defect(t *testing.T) {
	for _, id := range planted {
		ops := Counterexample(id)
		if len(ops) == 0 || len(ops) > 12 {
			t.Errorf("Counterexample(%q) has %d ops; want 1 to 12", id, len(ops))
			continue
		}
		good := play(newRef(), ops)
		if play(newRef(id), ops) == good {
			t.Errorf("Counterexample(%q) does not separate defect %s from the specification: both give %q", id, id, good)
		}
	}
}

func TestPool_agrees_with_the_specification(t *testing.T) {
	for seed := uint64(0); seed < 500; seed++ {
		ops := genOps(seed, 25)
		want := play(newRef(), ops)
		if got := play(NewPool(), ops); got != want {
			t.Fatalf("sequence %d diverges from the specification\nops: %+v\ngot : %s\nwant: %s", seed, ops, got, want)
		}
	}
}

func TestPool_spec_examples(t *testing.T) {
	p := NewPool()
	if p.Add("a", 100) != nil || p.Add("a", 1) == nil {
		t.Errorf("a balance of exactly Cap is allowed, one more is not")
	}
	if p.Use("a", 0) != nil || len(p.History("a")) != 1 {
		t.Errorf("Use(0) must succeed without a history entry")
	}
	if p.Use("a", 100) != nil || p.Report() != "a=0" {
		t.Errorf("using the whole balance must work, report %q", p.Report())
	}
	if p.Use("nobody", 1) == nil || strings.Contains(p.Report(), "nobody") {
		t.Errorf("Use on an unknown name must fail without creating it")
	}
}
