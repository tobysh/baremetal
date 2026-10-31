package main

import (
	"encoding/hex"
	"errors"
	"fmt"
	"runtime"
	"sort"
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

type lenCase struct {
	freq []int
	want []int
}

var lenKnown = []lenCase{{[]int{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, []int{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
	{[]int{3, 3, 3, 3}, []int{2, 2, 2, 2}},
	{[]int{1, 1, 2, 3, 5, 8, 13, 21}, []int{7, 7, 6, 5, 4, 3, 2, 1}},
	{[]int{1, 1, 2, 3, 5, 8, 13, 21, 34}, []int{8, 8, 7, 6, 5, 4, 3, 2, 1}},
	{[]int{0, 5, 0, 0, 9, 1, 0, 0, 2}, []int{0, 2, 0, 0, 1, 3, 0, 0, 3}},
	{[]int{7}, []int{1}},
	{[]int{0, 0}, []int{0, 0}},
	{[]int{4, 0, 4, 0, 4, 0, 4}, []int{2, 0, 2, 0, 2, 0, 2}},
	{[]int{5, 9, 12, 13, 16, 45}, []int{4, 4, 3, 3, 3, 1}},
	{[]int{1, 1, 1, 1, 1, 1, 1, 1}, []int{3, 3, 3, 3, 3, 3, 3, 3}},
}

func uniform(n, f int) []int {
	fr := make([]int, n)
	for i := range fr {
		fr[i] = f
	}
	return fr
}

// refCost is the optimal total cost sum(freq*len): the sum of all merged weights.
func refCost(freq []int) int {
	var ws []int
	for _, f := range freq {
		if f > 0 {
			ws = append(ws, f)
		}
	}
	cost := 0
	for len(ws) > 1 {
		sort.Ints(ws)
		s := ws[0] + ws[1]
		cost += s
		ws = append([]int{s}, ws[2:]...)
	}
	return cost
}

func TestBuildLengths_known_answers(t *testing.T) {
	for i, c := range lenKnown {
		got := BuildLengths(c.freq)
		if fmt.Sprint(got) != fmt.Sprint(c.want) {
			t.Errorf("case %d: freq %v\n got %v\nwant %v", i, c.freq, got, c.want)
		}
	}
}

func TestBuildLengths_special_cases(t *testing.T) {
	if got := BuildLengths([]int{0, 0, 0}); fmt.Sprint(got) != "[0 0 0]" {
		t.Errorf("no symbols: %v", got)
	}
	if got := BuildLengths([]int{0, 7, 0}); fmt.Sprint(got) != "[0 1 0]" {
		t.Errorf("one symbol must still get a 1-bit code: %v", got)
	}
	if got := BuildLengths(uniform(256, 3)); fmt.Sprint(got) != fmt.Sprint(uniform(256, 8)) {
		t.Errorf("256 equal frequencies should give 256 codes of 8 bits")
	}
}

func TestBuildLengths_optimal_and_complete_on_random_inputs(t *testing.T) {
	r := &tRand{s: 21}
	for round := 0; round < 300; round++ {
		freq := make([]int, 1+r.intn(40))
		used := 0
		for i := range freq {
			if r.intn(3) > 0 {
				freq[i] = 1 + r.intn(1+r.intn(50))
				used++
			}
		}
		got := BuildLengths(freq)
		cost, kraft := 0, 0.0
		for i, l := range got {
			if (freq[i] == 0) != (l == 0) {
				t.Fatalf("freq %v: symbol %d has length %d", freq, i, l)
			}
			cost += freq[i] * l
			if l > 0 {
				kraft += 1 / float64(uint64(1)<<uint(l))
			}
		}
		if used >= 2 && (kraft != 1.0 || cost != refCost(freq)) {
			t.Fatalf("freq %v: lengths %v have cost %d (optimal %d) and Kraft sum %v (must be 1)", freq, got, cost, refCost(freq), kraft)
		}
	}
}

func TestBuildLengths_is_deterministic_under_ties(t *testing.T) {
	// Every merge here is a tie: the rule (weight, then smallest symbol) fixes one answer.
	if got := BuildLengths([]int{1, 1, 1, 1, 1}); fmt.Sprint(got) != "[3 3 2 2 2]" {
		t.Errorf("five equal weights: %v; want [3 3 2 2 2] (smallest symbols merge first, so they end up deepest)", got)
	}
	if got := BuildLengths([]int{2, 1, 1, 2}); fmt.Sprint(got) != "[2 2 2 2]" {
		t.Errorf("[2 1 1 2]: %v; want [2 2 2 2]", got)
	}
}

func TestCanonicalCodes_rfc1951_example(t *testing.T) {
	// The worked example in RFC 1951 section 3.2.2: symbols A..H with these lengths.
	codes, err := CanonicalCodes([]int{3, 3, 3, 3, 3, 2, 4, 4})
	if err != nil {
		t.Fatal(err)
	}
	want := []string{"010", "011", "100", "101", "110", "00", "1110", "1111"}
	for i, c := range codes {
		if got := fmt.Sprintf("%0*b", c.Len, c.Bits); got != want[i] {
			t.Errorf("symbol %c: code %s; want %s", 'A'+i, got, want[i])
		}
	}
}

func TestCanonicalCodes_properties(t *testing.T) {
	codes, err := CanonicalCodes([]int{0, 1, 0, 0})
	if err != nil || codes[1].Len != 1 || codes[1].Bits != 0 || codes[0].Len != 0 {
		t.Errorf("a single used symbol: %v, %v; want code 0 of length 1 and Len 0 for the unused", codes, err)
	}
	r := &tRand{s: 31}
	for round := 0; round < 200; round++ {
		freq := make([]int, 2+r.intn(60))
		for i := range freq {
			freq[i] = r.intn(1 + r.intn(30))
		}
		lengths := BuildLengths(freq)
		codes, err := CanonicalCodes(lengths)
		if err != nil {
			t.Fatalf("lengths %v: %v", lengths, err)
		}
		for i, a := range codes {
			for j, b := range codes {
				if i == j || a.Len == 0 || b.Len == 0 || a.Len > b.Len {
					continue
				}
				if b.Bits>>uint(b.Len-a.Len) == a.Bits && !(a.Len == b.Len && i == j) {
					t.Fatalf("lengths %v: code of %d is a prefix of the code of %d", lengths, i, j)
				}
			}
			// canonical: among equal lengths, a later symbol has the next higher code
			for j := i + 1; j < len(codes); j++ {
				if codes[j].Len == a.Len && a.Len > 0 && codes[j].Bits <= a.Bits {
					t.Fatalf("lengths %v: codes of equal length not increasing with symbol (%d, %d)", lengths, i, j)
				}
			}
		}
	}
}

func TestCanonicalCodes_rejects_bad_lengths(t *testing.T) {
	for _, l := range [][]int{{1, 1, 1}, {2, 2, 2, 2, 2}, {-1, 2}, {49}, {1, 1, 49}} {
		if _, err := CanonicalCodes(l); !errors.Is(err, ErrBadLengths) {
			t.Errorf("CanonicalCodes(%v): err = %v; want ErrBadLengths", l, err)
		}
	}
	if _, err := CanonicalCodes([]int{1, 2}); err != nil {
		t.Errorf("an incomplete set is allowed: %v", err)
	}
}
