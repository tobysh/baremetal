package main

import "testing"

// refQuote is an independent restatement of the pricing behaviour that was
// shipped, written as a switch rather than a rule table.
func refQuote(p Parcel) (int, string) {
	var base int
	var name string
	switch {
	case p.Member && p.Zone == 1 && p.Grams <= 500 && !p.Express:
		return 0, "member-local"
	case p.Grams < 100 && p.Zone <= 2:
		name, base = "letter", 120+30*(p.Zone-1)
	case p.Zone == 3:
		name, base = "abroad", 900+250*((p.Grams+999)/1000)
	case p.Grams > 20000:
		name, base = "heavy", 2000+(p.Grams-20000)/10
	default:
		name, base = "standard", 300*p.Zone+40*((p.Grams+249)/250)
	}
	if p.Express {
		base += base / 2
	}
	if p.Member {
		base -= base / 10
	}
	if base < 100 {
		base = 100
	}
	if base > 5000 {
		base = 5000
	}
	return base, name
}

func TestQuote_is_unchanged(t *testing.T) {
	bad := 0
	for _, g := range []int{-5, 0, 1, 50, 99, 100, 101, 249, 250, 251, 499, 500, 501, 999, 1000, 1001, 2500, 2501, 19999, 20000, 20001, 30000, 100000} {
		for z := 0; z <= 4; z++ {
			for mask := 0; mask < 4; mask++ {
				p := Parcel{Grams: g, Zone: z, Express: mask&1 != 0, Member: mask&2 != 0}
				wc, wn := refQuote(p)
				gc, gn := Quote(p)
				if gc != wc || gn != wn {
					if bad < 5 {
						t.Errorf("Quote(%+v) = %d, %q; it was %d, %q before you started. This lesson is read-only", p, gc, gn, wc, wn)
					}
					bad++
				}
			}
		}
	}
	if bad > 0 {
		t.Errorf("%d parcels priced differently", bad)
	}
}

func TestAnswers(t *testing.T) {
	a := Answer()

	c, _ := refQuote(Parcel{Grams: 99, Zone: 2, Member: true})
	if a.A1 != c {
		t.Errorf("A1 = %d; not right. Follow the parcel through letter -> member discount -> clamps", a.A1)
	}
	_, n := refQuote(Parcel{Grams: 100, Zone: 1, Member: true, Express: true})
	if a.A2 != n {
		t.Errorf("A2 = %q; not right. Which rules can match grams=100 and express?", a.A2)
	}
	c, _ = refQuote(Parcel{Grams: 30000, Zone: 3})
	if a.A3 != c {
		t.Errorf("A3 = %d; not right. Check rule order and the maximum", a.A3)
	}
	_, n = refQuote(Parcel{Grams: 30000, Zone: 2})
	if a.A4 != n {
		t.Errorf("A4 = %q; not right", a.A4)
	}
	c, _ = refQuote(Parcel{Grams: 1, Zone: 0})
	if a.A5 != c {
		t.Errorf("A5 = %d; not right. Which rule matches zone 0, and what does the minimum do?", a.A5)
	}
	want := 0
	for g := 1; g < 100000; g++ {
		if c, _ := refQuote(Parcel{Grams: g, Zone: 2}); c > 1000 {
			want = g
			break
		}
	}
	if a.A6 != want {
		t.Errorf("A6 = %d; not right. Work out the standard price per 250 g step", a.A6)
	}
}
