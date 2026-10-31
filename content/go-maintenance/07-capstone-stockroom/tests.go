package main

import (
	"fmt"
	"sort"
	"strconv"
	"strings"
	"testing"
)

// ---- independent reference ------------------------------------------------

type refRow struct{ qty, price, res int }

type ref map[string]*refRow

func (r ref) adjust(s string, d int) bool {
	row, ok := r[s]
	switch {
	case d == 0:
		return true
	case !ok && d < 0:
		return false
	case !ok:
		r[s] = &refRow{qty: d}
		return true
	case row.qty+d < row.res:
		return false
	}
	row.qty += d
	return true
}

func (r ref) setPrice(s string, c int) bool {
	if row, ok := r[s]; ok && c >= 0 {
		row.price = c
		return true
	}
	return false
}

func (r ref) reserve(s string, n int) bool {
	row, ok := r[s]
	if !ok || n <= 0 || row.qty-row.res < n {
		return false
	}
	row.res += n
	return true
}

func (r ref) release(s string, n int) bool {
	row, ok := r[s]
	if !ok || n <= 0 || row.res < n {
		return false
	}
	row.res -= n
	return true
}

func (r ref) fulfil(s string, n int) bool {
	row, ok := r[s]
	if !ok || n <= 0 || row.res < n {
		return false
	}
	row.res -= n
	row.qty -= n
	return true
}

func (r ref) skus() []string {
	var k []string
	for s := range r {
		k = append(k, s)
	}
	sort.Strings(k)
	return k
}

func (r ref) save() string {
	var sb strings.Builder
	for _, s := range r.skus() {
		row := r[s]
		fmt.Fprintf(&sb, "%s,%d,%d", s, row.qty, row.price)
		if row.res > 0 {
			fmt.Fprintf(&sb, ",%d", row.res)
		}
		sb.WriteByte('\n')
	}
	return sb.String()
}

func (r ref) value() int {
	v := 0
	for _, row := range r {
		v += row.qty * row.price
	}
	return v
}

func (r ref) low(th int) []string {
	var k []string
	for _, s := range r.skus() { // sorted by sku first, then stable by qty
		if r[s].qty < th {
			k = append(k, s)
		}
	}
	sort.SliceStable(k, func(i, j int) bool { return r[k[i]].qty < r[k[j]].qty })
	return k
}

// ---- the previous release's loader (what older machines run) --------------

func oldLoad(data string) (map[string][2]int, error) {
	out := map[string][2]int{}
	for i, line := range strings.Split(data, "\n") {
		line = strings.TrimSpace(strings.TrimSuffix(line, "\r"))
		if line == "" || strings.HasPrefix(line, "#") {
			continue
		}
		f := strings.Split(line, ",")
		if len(f) != 2 && len(f) != 3 {
			return nil, fmt.Errorf("line %d: want 2 or 3 fields", i+1)
		}
		q, err := strconv.Atoi(strings.TrimSpace(f[1]))
		if err != nil {
			return nil, fmt.Errorf("line %d: bad quantity", i+1)
		}
		p := 0
		if len(f) == 3 {
			if p, err = strconv.Atoi(strings.TrimSpace(f[2])); err != nil {
				return nil, fmt.Errorf("line %d: bad price", i+1)
			}
		}
		out[strings.TrimSpace(f[0])] = [2]int{q, p}
	}
	return out, nil
}

// ---- helpers --------------------------------------------------------------

type rng uint64

func (r *rng) n(m int) int {
	*r = *r*6364136223846793005 + 1442695040888963407
	return int((uint64(*r) >> 33) % uint64(m))
}

func equalStrings(a, b []string) bool {
	return len(a) == len(b) && strings.Join(a, "\x00") == strings.Join(b, "\x00")
}

func load(t *testing.T, s string) *Inventory {
	t.Helper()
	inv, err := Load(s)
	if err != nil {
		t.Fatalf("Load(%q): %v", s, err)
	}
	return inv
}

// ---- regression: behaviour from before the change -------------------------

func TestRegression_load_and_save_formats(t *testing.T) {
	data := "# stock\r\n\r\nbolt,40\r\n  nut , 3 , 15 \nwasher,0,2\n"
	inv := load(t, data)
	if want := "bolt,40,0\nnut,3,15\nwasher,0,2\n"; inv.Save() != want {
		t.Errorf("Save = %q; want %q", inv.Save(), want)
	}
	if inv.Qty("nut") != 3 || inv.Qty("zzz") != 0 || inv.Value() != 45 {
		t.Errorf("Qty/Value wrong: %d %d", inv.Qty("nut"), inv.Value())
	}
	if got := load(t, "").Save(); got != "" {
		t.Errorf("empty Save = %q", got)
	}
}

func TestRegression_load_errors(t *testing.T) {
	for _, c := range []struct{ data, want string }{
		{"a,1\nb\n", "line 2: want 2 or 3 fields"},
		{"a,1,2,3,4\n", "line 1: want 2 or 3 fields"},
		{"\n,5\n", "line 2: empty sku"},
		{"a,x\n", "line 1: bad quantity"},
		{"a,-1\n", "line 1: bad quantity"},
		{"a,1,-2\n", "line 1: bad price"},
		{"a,1\n# c\na,2\n", "line 3: duplicate sku"},
	} {
		inv, err := Load(c.data)
		if err == nil || err.Error() != c.want || inv != nil {
			t.Errorf("Load(%q) = %v, %v; want error %q", c.data, inv, err, c.want)
		}
	}
}

func TestRegression_low_and_value_stay_physical(t *testing.T) {
	inv := load(t, "a,10,100,8\nb,3,50\nc,3,5\nd,20,1,19\ne,0\n")
	if got := inv.Low(11); !equalStrings(got, []string{"e", "b", "c", "a"}) {
		t.Errorf("Low(11) = %v; want [e b c a] (physical quantity, ties by sku)", got)
	}
	if got := inv.Value(); got != 10*100+3*50+3*5+20 {
		t.Errorf("Value = %d; reservations must not change it", got)
	}
	if got := inv.Low(0); len(got) != 0 {
		t.Errorf("Low(0) = %v", got)
	}
}

// ---- new behaviour --------------------------------------------------------

func TestReservations_basic(t *testing.T) {
	inv := load(t, "a,10,100\n")
	if inv.Reserve("a", 4) != nil || inv.Available("a") != 6 || inv.Qty("a") != 10 {
		t.Fatalf("Reserve 4: available %d qty %d", inv.Available("a"), inv.Qty("a"))
	}
	if inv.Reserve("a", 7) == nil || inv.Reserve("a", 0) == nil || inv.Reserve("zz", 1) == nil {
		t.Errorf("invalid reservations accepted")
	}
	if inv.Adjust("a", -7) == nil {
		t.Errorf("Adjust below reserved accepted")
	}
	if inv.Adjust("a", -6) != nil || inv.Qty("a") != 4 || inv.Available("a") != 0 {
		t.Errorf("Adjust to exactly the reserved amount must work")
	}
	if inv.Fulfil("a", 5) == nil || inv.Fulfil("a", 3) != nil || inv.Qty("a") != 1 || inv.Available("a") != 0 {
		t.Errorf("Fulfil wrong: qty %d avail %d", inv.Qty("a"), inv.Available("a"))
	}
	if inv.Release("a", 2) == nil || inv.Release("a", 1) != nil || inv.Available("a") != 1 {
		t.Errorf("Release wrong")
	}
	if inv.Available("nobody") != 0 {
		t.Errorf("Available of unknown sku")
	}
}

func TestStorage_oldest_format_that_fits(t *testing.T) {
	inv := load(t, "a,5,10\nb,7\n")
	plain := inv.Save()
	if plain != "a,5,10\nb,7,0\n" {
		t.Fatalf("no-reservation Save = %q; must be unchanged 3-field lines", plain)
	}
	if m, err := oldLoad(plain); err != nil || m["a"] != [2]int{5, 10} {
		t.Errorf("older release cannot read the new Save output: %v %v", m, err)
	}
	inv.Reserve("a", 2)
	if got, want := inv.Save(), "a,5,10,2\nb,7,0\n"; got != want {
		t.Errorf("Save with reservation = %q; want %q", got, want)
	}
	back := load(t, inv.Save())
	if back.Available("a") != 3 || back.Qty("a") != 5 || back.Save() != inv.Save() {
		t.Errorf("reservation lost in a save/load round trip")
	}
	inv.Release("a", 2)
	if _, err := oldLoad(inv.Save()); err != nil {
		t.Errorf("after releasing everything, output must be readable by the older release again: %v", err)
	}
}

func TestLoad_reserved_validation(t *testing.T) {
	for _, d := range []string{"a,5,1,6\n", "a,5,1,-1\n", "a,5,1,x\n", "a,5,1,\n"} {
		if inv, err := Load(d); err == nil || inv != nil || err.Error() != "line 1: bad reserved" {
			t.Errorf("Load(%q) = %v, %v; want line 1: bad reserved", d, inv, err)
		}
	}
	inv := load(t, "a,5,1,5\nb,2,2,0\n")
	if inv.Available("a") != 0 || inv.Available("b") != 2 || inv.Save() != "a,5,1,5\nb,2,2\n" {
		t.Errorf("load/save of reserved lines wrong: %q", inv.Save())
	}
}

func TestUpgrade(t *testing.T) {
	in := "# header\r\nbolt,40\r\n\r\n nut , 3 , 15 \nwasher,9,1,0\n; not a comment, a record?\nzed,4,4,2"
	if _, err := Upgrade(in); err == nil || !strings.HasPrefix(err.Error(), "line 6:") {
		t.Errorf("a line starting with ; is a record in this format and must be an error on line 6, got %v", err)
	}
	in = "# header\r\nbolt,40\r\n\r\n nut , 3 , 15 \nwasher,9,1,0\n# tail\nzed,4,4,2"
	want := "# header\nbolt,40,0\n\nnut,3,15\nwasher,9,1\n# tail\nzed,4,4,2"
	got, err := Upgrade(in)
	if err != nil || got != want {
		t.Fatalf("Upgrade = %q, %v\nwant %q", got, err, want)
	}
	again, err := Upgrade(got)
	if err != nil || again != got {
		t.Errorf("Upgrade is not idempotent: %q", again)
	}
	if a, b := load(t, in), load(t, got); a.Save() != b.Save() {
		t.Errorf("Upgrade changed the stock")
	}
	if out, _ := Upgrade("a,1\n"); out != "a,1,0\n" {
		t.Errorf("trailing newline lost: %q", out)
	}
}

func TestUpgrade_errors(t *testing.T) {
	for _, c := range []struct{ data, want string }{
		{"a,1\nb\n", "line 2: want 2 or 3 fields"},
		{"# x\na,1,2,9\n", "line 2: bad reserved"},
		{"a,1\n\na,2\n", "line 3: duplicate sku"},
		{"a,zz\n", "line 1: bad quantity"},
	} {
		out, err := Upgrade(c.data)
		if err == nil || out != "" || err.Error() != c.want {
			t.Errorf("Upgrade(%q) = %q, %v; want empty and %q", c.data, out, err, c.want)
		}
	}
}

// ---- differential: your module against the independent reference ----------

func TestAgainstReference_random_operations(t *testing.T) {
	skus := []string{"a", "b", "c"}
	deltas := []int{0, 1, 2, 5, -1, -2, -5, 10}
	for seed := 1; seed <= 300; seed++ {
		r := rng(seed * 104729)
		inv, want := NewInventory(), ref{}
		var trace []string
		for step := 0; step < 40; step++ {
			s := skus[r.n(3)]
			n := deltas[r.n(len(deltas))]
			var got, exp bool
			switch r.n(6) {
			case 0, 1:
				trace = append(trace, fmt.Sprintf("adjust %s %d", s, n))
				got, exp = inv.Adjust(s, n) == nil, want.adjust(s, n)
			case 2:
				trace = append(trace, fmt.Sprintf("price %s %d", s, n*3))
				got, exp = inv.SetPrice(s, n*3) == nil, want.setPrice(s, n*3)
			case 3:
				trace = append(trace, fmt.Sprintf("reserve %s %d", s, n))
				got, exp = inv.Reserve(s, n) == nil, want.reserve(s, n)
			case 4:
				trace = append(trace, fmt.Sprintf("release %s %d", s, n))
				got, exp = inv.Release(s, n) == nil, want.release(s, n)
			case 5:
				trace = append(trace, fmt.Sprintf("fulfil %s %d", s, n))
				got, exp = inv.Fulfil(s, n) == nil, want.fulfil(s, n)
			}
			last := trace[len(trace)-1]
			if got != exp {
				t.Fatalf("seed %d: %q succeeded=%v; reference says %v\ntrace: %v", seed, last, got, exp, trace)
			}
			if inv.Save() != want.save() {
				t.Fatalf("seed %d after %q: Save = %q; want %q", seed, last, inv.Save(), want.save())
			}
			for _, k := range skus {
				wantAvail := 0
				if row, ok := want[k]; ok {
					wantAvail = row.qty - row.res
				}
				if inv.Available(k) != wantAvail {
					t.Fatalf("seed %d after %q: Available(%s) = %d; want %d", seed, last, k, inv.Available(k), wantAvail)
				}
			}
			if inv.Value() != want.value() || !equalStrings(inv.Low(5), want.low(5)) {
				t.Fatalf("seed %d after %q: Value %d/%d Low %v/%v", seed, last, inv.Value(), want.value(), inv.Low(5), want.low(5))
			}
		}
		re := load(t, inv.Save())
		if re.Save() != inv.Save() {
			t.Fatalf("seed %d: save/load round trip changed the data", seed)
		}
		if up, err := Upgrade(inv.Save()); err != nil || up != inv.Save() {
			t.Fatalf("seed %d: Upgrade of canonical data changed it: %q %v", seed, up, err)
		}
	}
}
