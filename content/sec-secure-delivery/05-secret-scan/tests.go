package main

import (
	"math/rand"
	"reflect"
	"strings"
	"testing"
)

const alnum = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"

func tok(rng *rand.Rand, kind string, n int) string {
	b := make([]byte, n)
	for i := range b {
		b[i] = alnum[rng.Intn(len(alnum))]
	}
	return "stk_" + kind + "_" + string(b)
}

type want struct {
	Line, Col int
	Rule      string
}

func got(fs []Finding) []want {
	out := []want{}
	for _, f := range fs {
		out = append(out, want{f.Line, f.Col, f.Rule})
	}
	return out
}

func eq(t *testing.T, name string, fs []Finding, w []want) {
	t.Helper()
	if g := got(fs); !reflect.DeepEqual(g, w) && !(len(g) == 0 && len(w) == 0) {
		t.Errorf("%s:\n got  %v\n want %v", name, g, w)
	}
}

func TestTokensAndPositions(t *testing.T) {
	rng := rand.New(rand.NewSource(3))
	live, test := tok(rng, "live", 24), tok(rng, "test", 24)
	text := "# deploy notes\n" +
		"export KEY=" + live + "\n" +
		"  curl -H 'x: " + test + "' --verbose\n" +
		"two: " + live + " and " + test + "\n"
	eq(t, "tokens", Scan(text, nil), []want{{2, 12, "svc-token"}, {3, 15, "svc-token"}, {4, 6, "svc-token"}, {4, 6 + len(live) + 5, "svc-token"}})
}

func TestTokenNearMisses(t *testing.T) {
	rng := rand.New(rand.NewSource(4))
	for name, s := range map[string]string{
		"23 chars":      tok(rng, "live", 23),
		"25 chars":      tok(rng, "live", 25),
		"wrong env":     strings.Replace(tok(rng, "live", 24), "live", "prod", 1),
		"upper prefix":  "STK_LIVE_" + tok(rng, "x", 24)[6:],
		"underscore":    tok(rng, "live", 24) + "_x",
		"glued prefix":  "x" + tok(rng, "live", 24),
		"punctuated":    "stk_live_" + strings.Repeat("a", 12) + "-" + strings.Repeat("b", 11),
		"missing sep":   "stk_live" + strings.Repeat("a", 24),
	} {
		if fs := Scan("v="+s+"\n", nil); len(fs) != 0 {
			t.Errorf("%s: unexpected findings %v", name, got(fs))
		}
	}
	rng = rand.New(rand.NewSource(5))
	good := tok(rng, "test", 24)
	if fs := Scan("("+good+")", nil); len(fs) != 1 || fs[0].Col != 2 {
		t.Errorf("token in brackets: %v", fs)
	}
}

func TestPrivateKey(t *testing.T) {
	text := "ok\n  -----BEGIN SYNTHETIC PRIVATE KEY-----\nMIIBfake\n-----END SYNTHETIC PRIVATE KEY-----\n-----BEGIN SYNTHETIC PUBLIC KEY-----\n"
	eq(t, "key block", Scan(text, nil), []want{{2, 3, "private-key"}})
}

func TestAssignments(t *testing.T) {
	text := "" +
		"password = 'hunter2hunter2'\n" + // 1: reported, col of value
		"DB_PASSWORD: s3cr3t-Passw0rd\n" + // 2
		"api-key=Qw8rTy2uIo\n" + // 3
		"token_expiry = 3600\n" + // 4: too short
		"password=\"\"\n" + // 5
		"secret = ${SECRET_FROM_VAULT}\n" + // 6: placeholder
		"password: <your-password-here>\n" + // 7
		"API_KEY=env:SERVICE_KEY\n" + // 8
		"passwd = EXAMPLEvalue123\n" + // 9: contains example
		"secret=ChangeMe2024!!\n" + // 10: changeme
		"token = aaaaaaaabbbbb\n" + // 11: only 2 distinct bytes
		"client_secret_id: 12345678\n" + // 12: 8 distinct digits, reported
		"password=%PASSWORD%\r\n" + // 13: placeholder, CRLF
		"my_token =\tZ9x8C7v6B5n4\r\n" + // 14
		"description = a long sentence about secrets\n" // 15: name 'description', no match
	eq(t, "assignments", Scan(text, nil), []want{
		{1, 13, "assignment"}, {2, 14, "assignment"}, {3, 9, "assignment"}, {12, 19, "assignment"}, {14, 12, "assignment"},
	})
}

func TestTokenInsideAssignmentIsReportedOnce(t *testing.T) {
	rng := rand.New(rand.NewSource(8))
	k := tok(rng, "live", 24)
	eq(t, "single report", Scan("api_key = \""+k+"\"\n", nil), []want{{1, 12, "svc-token"}})
	eq(t, "token plus other assignment", Scan("token="+k+" password=Hq7wLm2xPz\n", nil), []want{{1, 7, "svc-token"}, {1, 6 + len(k) + 1 + 9 + 1, "assignment"}})
}

func TestBaselineSkipsTriagedSecrets(t *testing.T) {
	rng := rand.New(rand.NewSource(9))
	a, b := tok(rng, "live", 24), tok(rng, "test", 24)
	text := "a=" + a + "\nb=" + b + "\npassword=Hq7wLm2xPz\n"
	base := map[string]bool{Fingerprint(a): true, Fingerprint("Hq7wLm2xPz"): true}
	eq(t, "baseline", Scan(text, base), []want{{2, 3, "svc-token"}})
	eq(t, "empty baseline map", Scan(text, map[string]bool{}), []want{{1, 3, "svc-token"}, {2, 3, "svc-token"}, {3, 10, "assignment"}})
}

func TestReportsDoNotReLeak(t *testing.T) {
	rng := rand.New(rand.NewSource(10))
	a := tok(rng, "live", 24)
	text := "x=" + a + "\npassword = 'Hq7wLm2xPz-long-value'\n-----BEGIN SYNTHETIC PRIVATE KEY-----\n"
	fs := Scan(text, nil)
	if len(fs) != 3 {
		t.Fatalf("want 3 findings, got %v", got(fs))
	}
	for _, f := range fs {
		all := f.Rule + "|" + f.Preview + "|" + f.Fingerprint
		for _, secret := range []string{a, "Hq7wLm2xPz-long-value", "SYNTHETIC PRIVATE"} {
			if strings.Contains(all, secret[4:]) {
				t.Errorf("finding %+v exposes more than the first four bytes of a secret", f)
			}
		}
		if len(f.Fingerprint) != 12 {
			t.Errorf("fingerprint %q", f.Fingerprint)
		}
	}
	if fs[0].Fingerprint != Fingerprint(a) || fs[1].Fingerprint != Fingerprint("Hq7wLm2xPz-long-value") {
		t.Error("fingerprint must be of the secret itself (the token, the assignment value)")
	}
}

func TestCleanTextAndEmpty(t *testing.T) {
	eq(t, "empty", Scan("", nil), nil)
	eq(t, "prose", Scan("Rotate the password every 90 days.\nThe token service is down.\n", nil), nil)
}

func TestGeneratedFiles(t *testing.T) {
	// Plant secrets at known places among noise; the planted list is the oracle.
	rng := rand.New(rand.NewSource(21))
	for round := 0; round < 25; round++ {
		var lines []string
		var w []want
		for i := 1; i <= 30; i++ {
			switch rng.Intn(5) {
			case 0:
				prefix := strings.Repeat(" ", rng.Intn(6)) + "auth = "
				k := tok(rng, []string{"live", "test"}[rng.Intn(2)], 24)
				lines = append(lines, prefix+k+" # note")
				w = append(w, want{i, len(prefix) + 1, "svc-token"})
			case 1:
				lines = append(lines, "debug = "+tok(rng, "live", 24+1+rng.Intn(3)))
			case 2:
				lines = append(lines, "password = ${VAULT_"+alnum[rng.Intn(20):][:6]+"}")
			case 3:
				lines = append(lines, "level = info # line "+alnum[rng.Intn(40):][:8])
			default:
				lines = append(lines, "")
			}
		}
		eq(t, "round", Scan(strings.Join(lines, "\n"), nil), w)
	}
}
