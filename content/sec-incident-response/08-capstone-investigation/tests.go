package main

import (
	"fmt"
	"math/rand"
	"sort"
	"strings"
	"testing"
	"time"
)

type truthT struct {
	attack     bool
	victim, ip string
	want       map[string]bool // kind|subject
	idTime     map[string]int64
	authIP     map[string]string
	authOK     map[string]bool
	authT      map[string]int64
	exportQual map[string]int64 // id -> bytes
	T          int64
	privID     map[string]string // subject -> audit id
	fileID     map[string]bool
}

type gen struct {
	r     *rand.Rand
	truth *truthT
	c     Case
	na    int
	nw    int
	ng    int
}

var zones = []int{-8 * 3600, 0, 3600, 2 * 3600, 5*3600 + 1800, 9 * 3600}

func (g *gen) stamp(t int64) string {
	return time.Unix(t, 0).In(time.FixedZone("", zones[g.r.Intn(len(zones))])).Format(time.RFC3339)
}
func (g *gen) auth(t int64, user, ip string, ok bool) string {
	g.na++
	id := fmt.Sprint("A", g.na)
	res := "fail"
	if ok {
		res = "ok"
	}
	g.c.Auth = append(g.c.Auth, fmt.Sprintf("%s %s user=%s ip=%s %s", id, g.stamp(t), user, ip, res))
	g.truth.idTime[id], g.truth.authIP[id], g.truth.authOK[id], g.truth.authT[id] = t, ip, ok, t
	return id
}
func (g *gen) web(t int64, ip, user string, bytes int64, path string) string {
	g.nw++
	id := fmt.Sprint("W", g.nw)
	g.c.Web = append(g.c.Web, fmt.Sprintf("%s %s ip=%s user=%s bytes=%d %s", id, g.stamp(t), ip, user, bytes, path))
	g.truth.idTime[id] = t
	return id
}
func (g *gen) audit(t int64, actor, from, to string) string {
	g.ng++
	id := fmt.Sprint("G", g.ng)
	g.c.Audit = append(g.c.Audit, Audit{id, t, actor, from, to})
	g.c.After = append(g.c.After, Edge{from, to})
	g.truth.idTime[id] = t
	return id
}

func build(seed int64) (Case, *truthT) {
	r := rand.New(rand.NewSource(seed))
	tr := &truthT{want: map[string]bool{}, idTime: map[string]int64{}, authIP: map[string]string{}, authOK: map[string]bool{}, authT: map[string]int64{}, exportQual: map[string]int64{}, privID: map[string]string{}, fileID: map[string]bool{}}
	g := &gen{r: r, truth: tr}
	g.c.Allowed = map[string]string{}
	base := int64(1780000000 + r.Intn(100000))
	T := base + 40000 + int64(r.Intn(3000))
	attack := seed%4 != 3
	twoHop := seed%2 == 1
	exfil := seed%3 != 1
	keyPersist := seed%5 < 2
	users := []string{"alice", "bram", "chen", "dara", "emil", "fern"}
	victim := users[0]
	atkIP := fmt.Sprintf("203.0.113.%d", 100+r.Intn(100))
	home := func(k int) string { return fmt.Sprintf("198.51.100.%d", 10+k) }
	tr.attack, tr.T = attack, T
	if attack {
		tr.victim, tr.ip = victim, atkIP
	}
	// legitimate traffic
	for k, u := range users {
		for n := 0; n < 3; n++ {
			t := base + int64(r.Intn(80000))
			if r.Intn(3) == 0 {
				g.auth(t-30, u, home(k), false)
			}
			g.auth(t, u, home(k), true)
			g.web(t+5, home(k), u, int64(500+r.Intn(5000)), "/app/dashboard")
		}
	}
	g.web(T-3*3600, home(0), victim, 5000000, "/export/monthly.csv") // legit big export
	g.web(T+3600, home(2), "chen", 2000000, "/export/finance.csv")   // other staff, after T
	// decoy logins
	ipB, ipC, ipD, ipE := "203.0.113.50", "203.0.113.51", "203.0.113.52", "203.0.113.53"
	tb := base + 20000 + int64(r.Intn(40000))
	for k := 0; k < 4; k++ {
		g.auth(tb+int64(k*40), users[k%6], ipB, false)
	}
	g.auth(tb+200, users[1], ipB, true) // 4 fails then success
	tc := base + 15000 + int64(r.Intn(40000))
	for k := 0; k < 6; k++ {
		g.auth(tc+int64(k*700), users[k%6], ipC, false)
	}
	g.auth(tc+5*700+100, users[2], ipC, true) // only one failure in window
	for k := 0; k < 9; k++ {
		g.auth(base+30000+int64(k*20), users[k%6], ipD, false)
	}
	g.web(T+7200, ipD, "-", 3000000, "/export/scan.csv")
	te := base + 10000 + int64(r.Intn(40000))
	for k := 0; k < 7; k++ {
		g.auth(te+int64(k*40), users[k%6], ipE, false)
	}
	g.auth(te+400, users[3], home(3), true) // success from another address
	// files
	hsh := func(s string) string { return fmt.Sprintf("h-%s-%d", s, seed) }
	B := func(p, h string, m int64) { g.c.Base = append(g.c.Base, File{p, hsh(h), "root", 0644, m, m}) }
	N := func(p, h string, mt, ct int64) { g.c.Now = append(g.c.Now, File{p, hsh(h), "root", 0644, mt, ct}) }
	old := base - 30*86400
	for k := 0; k < 8; k++ {
		p := fmt.Sprintf("/usr/lib/fill%d", k)
		B(p, "fill", old)
		N(p, "fill", old, old)
	}
	keys := "/home/" + victim + "/.ssh/authorized_keys"
	B(keys, "k0", old)
	B("/etc/cron.d/logrotate-extra", "l0", old)
	B("/etc/cron.d/backup", "b0", old)
	B("/etc/cron.d/old-job", "o0", old)
	B("/etc/systemd/system/app.service", "s0", old)
	N("/etc/cron.d/logrotate-extra", "l1", base-7*86400, base-7*86400) // earlier maintenance change
	N("/etc/cron.d/backup", "b0", T+500, T+500)                        // touched only
	N("/etc/systemd/system/app.service", "s1", T+900, T+900)           // package update
	g.c.Allowed["/etc/systemd/system/app.service"] = hsh("s1")
	N("/etc/cron.d/patch-window", "pw", base+1000, base+1000) // added before intrusion
	N("/var/tmp/cache.bin", "cb", T+1200, T+1200)             // not autostart
	if attack {
		N(keys, "k0", old, old)
	} else {
		N(keys, "k0", old, old)
		N("/etc/cron.d/late-admin-job", "lj", T+300, T+300) // quiet case: no intrusion
	}
	if attack {
		// initial access
		nf := 5 + int(seed%4)
		var ids []string
		for k := 0; k < nf; k++ {
			ts := T - 1 - int64(r.Intn(590))
			if k == 0 && seed%3 == 0 {
				ts = T - 600
			}
			ids = append(ids, g.auth(ts, users[(k+1)%6], atkIP, false))
		}
		if seed%2 == 0 {
			g.auth(T-601, victim, atkIP, false) // outside the window
		}
		g.auth(T, victim, atkIP, true)
		tr.want["initial-access|"+victim] = true
		_ = ids
		// persistence
		cron := fmt.Sprintf("/etc/cron.d/sync%d", seed)
		mt := old
		if seed%2 == 1 {
			mt = T + 400
		}
		N(cron, "evil-cron", mt, T+400)
		tr.want["persistence|"+cron] = true
		tr.fileID["F:"+cron] = true
		tr.idTime["F:"+cron] = T + 400
		if keyPersist {
			g.c.Now[indexOf(g.c.Now, keys)] = File{keys, hsh("k1"), "root", 0644, old, T + 800}
			tr.want["persistence|"+keys] = true
			tr.fileID["F:"+keys] = true
			tr.idTime["F:"+keys] = T + 800
		}
		// graph
		g.c.Before = []Edge{{victim, "helpdesk"}, {"helpdesk", "ops"}, {"ops", "admins"}, {"dev1", "devs"}, {"loopA", "loopB"}, {"loopB", "loopA"}}
		g.c.After = append([]Edge{}, g.c.Before...)
		if twoHop {
			a := g.audit(T+600, victim, "svc-tmp", "grp-x")
			b := g.audit(T+640, victim, "grp-x", "admins")
			tr.want["privilege-change|svc-tmp>grp-x"], tr.want["privilege-change|grp-x>admins"] = true, true
			tr.privID["svc-tmp>grp-x"], tr.privID["grp-x>admins"] = a, b
		} else {
			a := g.audit(T+600, victim, "svc-tmp", "ops")
			tr.want["privilege-change|svc-tmp>ops"] = true
			tr.privID["svc-tmp>ops"] = a
		}
		g.audit(T+700, victim, "svc-bak", "wiki") // no route to admins
		g.audit(T-5000, victim, "temp2", "ops")   // before intrusion (but it is an enabler by structure)
		g.audit(T+800, victim, victim, "admins")  // redundant for an already privileged user
		g.audit(T+900, "carol", "newhire", "ops") // another actor, legitimate
		// web: recon, pre-T export, post-T activity
		g.web(T-100, atkIP, "-", 2000000, "/export/probe.csv")
		g.web(T+20, atkIP, victim, 800, "/admin/users")
		g.web(T+30, atkIP, victim, 120, "/static/logo.png")
		if exfil {
			for k := 0; k < 3+r.Intn(3); k++ {
				id := g.web(T+1000+int64(k*60), atkIP, victim, int64(350000+r.Intn(150000)), fmt.Sprintf("/export/customers-%d.csv", k))
				tr.exportQual[id] = 1
			}
			tr.want["exfiltration|"+atkIP] = true
		} else {
			g.web(T+1000, atkIP, victim, 100000, "/export/a.csv")
			g.web(T+1060, atkIP, victim, 100000, "/export/b.csv")
		}
	} else {
		g.c.Before = []Edge{{victim, "helpdesk"}, {"helpdesk", "ops"}, {"ops", "admins"}}
		g.c.After = append([]Edge{}, g.c.Before...)
		g.audit(T+900, "carol", "newhire", "ops")
		g.audit(T+950, victim, "temp3", "ops")
	}
	// sort of the lists: deterministic shuffle
	r.Shuffle(len(g.c.Auth), func(i, j int) { g.c.Auth[i], g.c.Auth[j] = g.c.Auth[j], g.c.Auth[i] })
	r.Shuffle(len(g.c.Web), func(i, j int) { g.c.Web[i], g.c.Web[j] = g.c.Web[j], g.c.Web[i] })
	r.Shuffle(len(g.c.Now), func(i, j int) { g.c.Now[i], g.c.Now[j] = g.c.Now[j], g.c.Now[i] })
	r.Shuffle(len(g.c.Audit), func(i, j int) { g.c.Audit[i], g.c.Audit[j] = g.c.Audit[j], g.c.Audit[i] })
	// exfil qualifying bytes
	for id := range tr.exportQual {
		for _, l := range g.c.Web {
			if strings.HasPrefix(l, id+" ") {
				var b int64
				fmt.Sscanf(l[strings.Index(l, "bytes=")+6:], "%d", &b)
				tr.exportQual[id] = b
			}
		}
	}
	return g.c, tr
}

func indexOf(fs []File, p string) int {
	for i, f := range fs {
		if f.Path == p {
			return i
		}
	}
	return -1
}

func TestPublicTiny(t *testing.T) {
	var c Case
	for i := 0; i < 5; i++ {
		c.Auth = append(c.Auth, fmt.Sprintf("A%d 2026-03-04T10:00:%02dZ user=x ip=192.0.2.1 fail", i+1, i*5))
	}
	c.Auth = append(c.Auth, "A6 2026-03-04T11:00:40+01:00 user=kim ip=192.0.2.1 ok")
	rep := Investigate(c)
	if len(rep.Findings) != 1 || rep.Findings[0].Kind != "initial-access" || rep.Findings[0].Subject != "kim" || len(rep.Findings[0].Cites) != 6 {
		t.Fatal(rep)
	}
}

func TestInvestigations(t *testing.T) {
	for seed := int64(0); seed < 40; seed++ {
		c, tr := build(seed)
		rep := Investigate(c)
		if !tr.attack {
			if len(rep.Findings) != 0 || len(rep.Timeline) != 0 {
				t.Fatalf("seed %d: quiet case must report nothing, got %+v", seed, rep)
			}
			continue
		}
		got := map[string]Finding{}
		var keys []string
		for _, f := range rep.Findings {
			k := f.Kind + "|" + f.Subject
			if _, dup := got[k]; dup {
				t.Fatalf("seed %d: duplicate finding %s", seed, k)
			}
			got[k] = f
			keys = append(keys, k)
			seen := map[string]bool{}
			for _, id := range f.Cites {
				if seen[id] {
					t.Fatalf("seed %d: %s cites %s twice", seed, k, id)
				}
				seen[id] = true
				if _, ok := tr.idTime[id]; !ok {
					t.Fatalf("seed %d: %s cites unknown evidence %s", seed, k, id)
				}
			}
		}
		if !sort.StringsAreSorted(keys) {
			t.Fatalf("seed %d: findings not sorted by kind then subject: %v", seed, keys)
		}
		for k := range tr.want {
			if _, ok := got[k]; !ok {
				t.Fatalf("seed %d: missing finding %s (have %v)", seed, k, keys)
			}
		}
		for k := range got {
			if !tr.want[k] {
				t.Fatalf("seed %d: unexpected finding %s: a decoy was reported", seed, k)
			}
		}
		for k, f := range got {
			kind, subj, _ := strings.Cut(k, "|")
			switch kind {
			case "initial-access":
				succ, inwin := 0, 0
				for _, id := range f.Cites {
					if tr.authOK[id] {
						succ++
						if tr.authT[id] != tr.T || tr.authIP[id] != tr.ip {
							t.Fatalf("seed %d: wrong success cited %s", seed, id)
						}
					} else {
						if _, ok := tr.authT[id]; !ok || tr.authIP[id] != tr.ip || tr.authT[id] >= tr.T {
							t.Fatalf("seed %d: cited failure %s is not from the attacker before access", seed, id)
						}
						if tr.authT[id] >= tr.T-600 {
							inwin++
						}
					}
				}
				if succ != 1 || inwin < 5 {
					t.Fatalf("seed %d: initial-access needs the success and >=5 in-window failures, got %d/%d", seed, succ, inwin)
				}
			case "persistence":
				if len(f.Cites) != 1 || f.Cites[0] != "F:"+subj {
					t.Fatalf("seed %d: persistence %s cites %v", seed, subj, f.Cites)
				}
			case "privilege-change":
				if len(f.Cites) != 1 || f.Cites[0] != tr.privID[subj] {
					t.Fatalf("seed %d: privilege %s cites %v want %s", seed, subj, f.Cites, tr.privID[subj])
				}
			case "exfiltration":
				var sum int64
				for _, id := range f.Cites {
					b, ok := tr.exportQual[id]
					if !ok {
						t.Fatalf("seed %d: exfiltration cites non-qualifying %s", seed, id)
					}
					sum += b
				}
				if sum < 1000000 {
					t.Fatalf("seed %d: cited exports sum to %d", seed, sum)
				}
			}
		}
		// timeline = union of cites, ordered by true UTC time
		set := map[string]bool{}
		var want []string
		for _, f := range rep.Findings {
			for _, id := range f.Cites {
				if !set[id] {
					set[id] = true
					want = append(want, id)
				}
			}
		}
		sort.Slice(want, func(i, j int) bool {
			a, b := tr.idTime[want[i]], tr.idTime[want[j]]
			if a != b {
				return a < b
			}
			return want[i] < want[j]
		})
		if fmt.Sprint(rep.Timeline) != fmt.Sprint(want) {
			t.Fatalf("seed %d: timeline\n got %v\nwant %v", seed, rep.Timeline, want)
		}
		if len(rep.Assumptions) == 0 {
			t.Fatalf("seed %d: write at least one assumption (content is not graded)", seed)
		}
	}
}
