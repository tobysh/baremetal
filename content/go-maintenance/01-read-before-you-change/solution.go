package main

import "fmt"

// Parcel describes something to be shipped.
type Parcel struct {
	Grams   int
	Zone    int // 1 = local, 2 = national, 3 = abroad
	Express bool
	Member  bool
}

const (
	minCents = 100
	maxCents = 5000
)

// A rule prices a parcel when match returns true.
type rule struct {
	name  string
	match func(Parcel) bool
	base  func(Parcel) int
}

// Rules are tried in order; the first one that matches wins.
var rules = []rule{
	{
		name:  "member-local",
		match: func(p Parcel) bool { return p.Member && p.Zone == 1 && p.Grams <= 500 && !p.Express },
		base:  func(p Parcel) int { return 0 },
	},
	{
		name:  "letter",
		match: func(p Parcel) bool { return p.Grams < 100 && p.Zone <= 2 },
		base:  func(p Parcel) int { return 120 + 30*(p.Zone-1) },
	},
	{
		name:  "abroad",
		match: func(p Parcel) bool { return p.Zone == 3 },
		base:  func(p Parcel) int { return 900 + 250*((p.Grams+999)/1000) },
	},
	{
		name:  "heavy",
		match: func(p Parcel) bool { return p.Grams > 20000 },
		base:  func(p Parcel) int { return 2000 + (p.Grams-20000)/10 },
	},
	{
		name:  "standard",
		match: func(p Parcel) bool { return true },
		base:  func(p Parcel) int { return 300*p.Zone + 40*((p.Grams+249)/250) },
	},
}

// Quote returns the price in cents and the name of the rule that set the base price.
func Quote(p Parcel) (int, string) {
	for _, r := range rules {
		if !r.match(p) {
			continue
		}
		c := r.base(p)
		if r.name == "member-local" {
			return c, r.name // free: no surcharge, no minimum
		}
		if p.Express {
			c += c / 2
		}
		if p.Member {
			c -= c / 10
		}
		if c < minCents {
			c = minCents
		}
		if c > maxCents {
			c = maxCents
		}
		return c, r.name
	}
	return 0, "none"
}

// Describe formats a quote for a receipt line.
func Describe(p Parcel) string {
	c, name := Quote(p)
	return fmt.Sprintf("%d.%02d via %s", c/100, c%100, name)
}

// Answers holds your answers to the six questions in the task.
type Answers struct {
	A1 int
	A2 string
	A3 int
	A4 string
	A5 int
	A6 int
}

// Answer returns your answers. Fill these in from reading the code above.
func Answer() Answers {
	return Answers{
		A1: 135,
		A2: "standard",
		A3: 5000,
		A4: "heavy",
		A5: 100,
		A6: 2501,
	}
}

func main() {
	fmt.Println(Describe(Parcel{Grams: 250, Zone: 2}))
	fmt.Println(Describe(Parcel{Grams: 40, Zone: 1, Member: true, Express: true}))
	fmt.Printf("%+v\n", Answer())
}
