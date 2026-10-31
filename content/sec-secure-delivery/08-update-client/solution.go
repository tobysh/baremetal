package main

import (
	"bytes"
	"crypto/ed25519"
	"crypto/sha256"
	"errors"
	"fmt"
	"strconv"
	"strings"
)

// ---------------------------------------------------------------- errors

var (
	ErrBadSignature  = errors.New("bad signature")
	ErrBadMeta       = errors.New("malformed metadata")
	ErrBodyMismatch  = errors.New("package body does not match signed metadata")
	ErrWriteVerify   = errors.New("data read back differs from what was written")
	ErrUnhealthy     = errors.New("new version failed its health check")
	ErrNoImage       = errors.New("no bootable image")
	ErrCorruptState  = errors.New("update state is corrupt")
	ErrIO            = errors.New("i/o error")
	ErrCrashed       = errors.New("process crashed")
	ErrNotExist      = errors.New("file does not exist")
	ErrMalformed     = errors.New("malformed offer")
	ErrFuture        = errors.New("offer issued in the future")
	ErrExpired       = errors.New("offer expired")
	ErrStaleMetadata = errors.New("offer older than metadata already seen")
	ErrAlreadyInst   = errors.New("already installed")
	ErrEquivocation  = errors.New("same counter, different content")
	ErrDowngrade     = errors.New("downgrade refused")
)

// ------------------------------------------- simulated disk with faults

type Fault int

const (
	FaultNone       Fault = iota
	FaultError            // the operation fails and changes nothing
	FaultTorn             // Write stores half the data and fails; Rename/Remove behave like FaultError
	FaultCrash            // the process dies before the operation happens
	FaultTornCrash        // like FaultTorn, and the process dies
	FaultCrashAfter       // the operation completes, then the process dies
	FaultLie              // Write stores half the data but reports success; no effect on Rename/Remove
)

// Disk is an in-memory file store. Every Write, Rename and Remove is a numbered
// "mutating operation"; Plan maps an operation number (1-based, counted in Ops) to
// the Fault to inject there. Write is NOT atomic. Rename IS atomic and replaces
// the target. After a crash every call fails with ErrCrashed until Restart.
type Disk struct {
	files map[string][]byte
	Ops   int
	Plan  map[int]Fault
	dead  bool
}

func NewDisk() *Disk { return &Disk{files: map[string][]byte{}} }

func (d *Disk) begin() (Fault, error) {
	if d.dead {
		return FaultNone, ErrCrashed
	}
	d.Ops++
	return d.Plan[d.Ops], nil
}

func (d *Disk) Read(name string) ([]byte, bool) {
	b, ok := d.files[name]
	if d.dead || !ok {
		return nil, false
	}
	return append([]byte(nil), b...), true
}

func (d *Disk) Write(name string, data []byte) error {
	f, err := d.begin()
	if err != nil {
		return err
	}
	switch f {
	case FaultError:
		return ErrIO
	case FaultCrash:
		d.dead = true
		return ErrCrashed
	case FaultTorn, FaultTornCrash, FaultLie:
		d.files[name] = append([]byte(nil), data[:len(data)/2]...)
		switch f {
		case FaultTorn:
			return ErrIO
		case FaultTornCrash:
			d.dead = true
			return ErrCrashed
		}
		return nil
	}
	d.files[name] = append([]byte(nil), data...)
	if f == FaultCrashAfter {
		d.dead = true
		return ErrCrashed
	}
	return nil
}

func (d *Disk) Rename(from, to string) error {
	f, err := d.begin()
	if err != nil {
		return err
	}
	switch f {
	case FaultError, FaultTorn:
		return ErrIO
	case FaultCrash, FaultTornCrash:
		d.dead = true
		return ErrCrashed
	}
	b, ok := d.files[from]
	if !ok {
		return ErrNotExist
	}
	d.files[to] = b
	delete(d.files, from)
	if f == FaultCrashAfter {
		d.dead = true
		return ErrCrashed
	}
	return nil
}

func (d *Disk) Remove(name string) error {
	f, err := d.begin()
	if err != nil {
		return err
	}
	switch f {
	case FaultError, FaultTorn:
		return ErrIO
	case FaultCrash, FaultTornCrash:
		d.dead = true
		return ErrCrashed
	}
	delete(d.files, name)
	if f == FaultCrashAfter {
		d.dead = true
		return ErrCrashed
	}
	return nil
}

// Restart brings a crashed process back. Files stay as the crash left them.
func (d *Disk) Restart() { d.dead = false }

// Corrupt flips one bit in a stored file (bit rot).
func (d *Disk) Corrupt(name string) {
	if b := d.files[name]; len(b) > 0 {
		b[len(b)/2] ^= 0x10
	}
}

// Snapshot returns a copy of every file, for comparisons.
func (d *Disk) Snapshot() map[string]string {
	out := map[string]string{}
	for k, v := range d.files {
		out[k] = string(v)
	}
	return out
}

// ---------------------------------------------- packages and metadata

type Package struct {
	Meta []byte // signed text, see ParseMeta
	Sig  []byte // ed25519 signature over Meta
	Body []byte
}

type Meta struct {
	Version         string
	Counter         uint64
	Issued, Expires int64
	Size            int
	Digest          string // "sha256:<hex>"
}

// ParseMeta reads the strict six-line format
//
//	version=<text>\ncounter=<n>\nissued=<n>\nexpires=<n>\nsize=<n>\ndigest=sha256:<hex>\n
func ParseMeta(b []byte) (Meta, error) {
	var m Meta
	lines := strings.Split(string(b), "\n")
	if len(lines) != 7 || lines[6] != "" {
		return m, ErrBadMeta
	}
	keys := []string{"version", "counter", "issued", "expires", "size", "digest"}
	vals := make([]string, 6)
	for i, k := range keys {
		v, ok := strings.CutPrefix(lines[i], k+"=")
		if !ok || v == "" {
			return m, ErrBadMeta
		}
		vals[i] = v
	}
	c, e1 := strconv.ParseUint(vals[1], 10, 63)
	is, e2 := strconv.ParseInt(vals[2], 10, 64)
	ex, e3 := strconv.ParseInt(vals[3], 10, 64)
	sz, e4 := strconv.Atoi(vals[4])
	d, ok := strings.CutPrefix(vals[5], "sha256:")
	if e1 != nil || e2 != nil || e3 != nil || e4 != nil || sz < 0 || !ok || len(d) != 64 || strings.Trim(d, "0123456789abcdef") != "" {
		return m, ErrBadMeta
	}
	return Meta{vals[0], c, is, ex, sz, vals[5]}, nil
}

// ------------------------------------- freshness decision (lesson 7)

const Skew int64 = 300

type State struct {
	Counter, HighWater uint64
	Digest             string
	LastIssued         int64
}

type Offer struct {
	Version             string
	Counter             uint64
	IssuedAt, ExpiresAt int64
	Digest              string
}

func Decide(st State, o Offer, now int64) error {
	switch {
	case o.Version == "" || o.Digest == "" || o.Counter == 0 || o.ExpiresAt <= o.IssuedAt:
		return ErrMalformed
	case o.IssuedAt > now+Skew:
		return ErrFuture
	case now >= o.ExpiresAt:
		return ErrExpired
	case o.IssuedAt < st.LastIssued:
		return ErrStaleMetadata
	case o.Counter == st.Counter && o.Digest == st.Digest:
		return ErrAlreadyInst
	case o.Counter == st.Counter:
		return ErrEquivocation
	case o.Counter <= st.HighWater || o.Counter < st.Counter:
		return ErrDowngrade
	}
	return nil
}

// --------------------------------------------- on-disk layout (supplied)
//
//   slot.a, slot.b   package bodies (two slots; one is running, one is spare)
//   active           which slot is current and which is the previous good one
//   hw               high-water mark: highest counter ever committed + newest issue time
//
// "active" and "hw" are only ever replaced with atomicWrite, so they are either the
// old content or the new content, never a mixture.

const (
	SlotA      = "slot.a"
	SlotB      = "slot.b"
	ActiveFile = "active"
	HWFile     = "hw"
)

func otherSlot(s string) string {
	if s == SlotA {
		return SlotB
	}
	return SlotA
}

type Record struct {
	Slot            string
	Counter         uint64
	Issued          int64
	Version, Digest string
}

type Active struct {
	Cur  Record
	Prev *Record // nil when there is no previous good version
}

func (r Record) line(tag string) string {
	return fmt.Sprintf("%s %s %d %d %s %s\n", tag, r.Slot, r.Counter, r.Issued, r.Version, r.Digest)
}

func encodeActive(a Active) []byte {
	s := a.Cur.line("cur")
	if a.Prev == nil {
		s += "prev none\n"
	} else {
		s += a.Prev.line("prev")
	}
	return []byte(s)
}

func parseRecord(line, tag string) (Record, bool) {
	f := strings.Split(line, " ")
	if len(f) != 6 || f[0] != tag || (f[1] != SlotA && f[1] != SlotB) {
		return Record{}, false
	}
	c, e1 := strconv.ParseUint(f[2], 10, 63)
	i, e2 := strconv.ParseInt(f[3], 10, 64)
	if e1 != nil || e2 != nil || f[4] == "" || !strings.HasPrefix(f[5], "sha256:") {
		return Record{}, false
	}
	return Record{f[1], c, i, f[4], f[5]}, true
}

func parseActive(b []byte) (Active, error) {
	lines := strings.Split(string(b), "\n")
	if len(lines) != 3 || lines[2] != "" {
		return Active{}, ErrCorruptState
	}
	cur, ok := parseRecord(lines[0], "cur")
	if !ok {
		return Active{}, ErrCorruptState
	}
	if lines[1] == "prev none" {
		return Active{Cur: cur}, nil
	}
	prev, ok := parseRecord(lines[1], "prev")
	if !ok || prev.Slot == cur.Slot {
		return Active{}, ErrCorruptState
	}
	return Active{cur, &prev}, nil
}

type HW struct {
	Counter uint64
	Issued  int64
}

func encodeHW(h HW) []byte { return []byte(fmt.Sprintf("%d %d\n", h.Counter, h.Issued)) }

// readHW: a missing file is the zero mark; an unparsable one is ErrCorruptState.
func readHW(d *Disk) (HW, error) {
	b, ok := d.Read(HWFile)
	if !ok {
		return HW{}, nil
	}
	var h HW
	if n, err := fmt.Sscanf(string(b), "%d %d\n", &h.Counter, &h.Issued); n != 2 || err != nil || !bytes.Equal(b, encodeHW(h)) {
		return HW{}, ErrCorruptState
	}
	return h, nil
}

// readActive: ok=false when the device has never been provisioned.
func readActive(d *Disk) (a Active, ok bool, err error) {
	b, present := d.Read(ActiveFile)
	if !present {
		return Active{}, false, nil
	}
	a, err = parseActive(b)
	return a, err == nil, err
}

// atomicWrite writes name+".tmp", reads it back, and only then renames it over
// name. A failure leaves the old content of name untouched (and maybe a stray tmp file).
func atomicWrite(d *Disk, name string, data []byte) error {
	tmp := name + ".tmp"
	if err := d.Write(tmp, data); err != nil {
		return err
	}
	if back, _ := d.Read(tmp); !bytes.Equal(back, data) {
		return ErrWriteVerify
	}
	return d.Rename(tmp, name)
}

// raiseHW makes the high-water mark at least (counter, issued). It writes nothing if it is already there.
func raiseHW(d *Disk, counter uint64, issued int64) error {
	h, err := readHW(d)
	if err != nil {
		return err
	}
	n := h
	if counter > n.Counter {
		n.Counter = counter
	}
	if issued > n.Issued {
		n.Issued = issued
	}
	if n == h {
		return nil
	}
	return atomicWrite(d, HWFile, encodeHW(n))
}

// LoadState derives the freshness State from the disk.
func LoadState(d *Disk) (State, error) {
	h, err := readHW(d)
	if err != nil {
		return State{}, err
	}
	a, ok, err := readActive(d)
	if err != nil {
		return State{}, err
	}
	st := State{HighWater: h.Counter, LastIssued: h.Issued}
	if ok {
		st.Counter, st.Digest = a.Cur.Counter, a.Cur.Digest
		if a.Cur.Counter > st.HighWater {
			st.HighWater = a.Cur.Counter
		}
		if a.Cur.Issued > st.LastIssued {
			st.LastIssued = a.Cur.Issued
		}
	}
	return st, nil
}

func digestOf(b []byte) string { return fmt.Sprintf("sha256:%x", sha256.Sum256(b)) }

// ------------------------------------------------------- YOUR PART

type Config struct {
	Pub    ed25519.PublicKey
	Health func(body []byte) bool // nil means "always healthy"
}

type Image struct {
	Slot    string
	Version string
	Counter uint64
	Body    []byte
}

// Verify checks a package against the trusted key and the freshness state, with no
// side effects. Order: signature (ErrBadSignature), metadata (ErrBadMeta),
// Decide (its errors, unchanged), then body size and digest (ErrBodyMismatch).
func Verify(cfg Config, st State, pkg Package, now int64) (Meta, error) {
	if len(cfg.Pub) != ed25519.PublicKeySize || !ed25519.Verify(cfg.Pub, pkg.Meta, pkg.Sig) {
		return Meta{}, ErrBadSignature
	}
	m, err := ParseMeta(pkg.Meta)
	if err != nil {
		return Meta{}, err
	}
	if err := Decide(st, Offer{m.Version, m.Counter, m.Issued, m.Expires, m.Digest}, now); err != nil {
		return Meta{}, err
	}
	if len(pkg.Body) != m.Size || digestOf(pkg.Body) != m.Digest {
		return Meta{}, ErrBodyMismatch
	}
	return m, nil
}

// Recover repairs what an interrupted run may have left: stray active.tmp, hw.tmp
// files (remove them, only if present) and a high-water mark that is behind the
// active version (raise it). It must change nothing on a clean device.
func Recover(d *Disk) error {
	for _, tmp := range []string{ActiveFile + ".tmp", HWFile + ".tmp"} {
		if _, ok := d.Read(tmp); ok {
			if err := d.Remove(tmp); err != nil {
				return err
			}
		}
	}
	a, ok, err := readActive(d)
	if err != nil || !ok {
		return err
	}
	return raiseHW(d, a.Cur.Counter, a.Cur.Issued)
}

// Rollback abandons the current version. Order: first raise the high-water mark to
// the abandoned counter, then commit the previous version (cur=prev, prev=none).
// With no previous version, remove the active file instead. If a previous version
// exists but its slot no longer matches its digest, return ErrNoImage and change nothing.
func Rollback(d *Disk) error {
	a, ok, err := readActive(d)
	if err != nil || !ok {
		return err
	}
	if a.Prev == nil {
		if err := raiseHW(d, a.Cur.Counter, a.Cur.Issued); err != nil {
			return err
		}
		return d.Remove(ActiveFile)
	}
	body, _ := d.Read(a.Prev.Slot)
	if digestOf(body) != a.Prev.Digest {
		return ErrNoImage
	}
	if err := raiseHW(d, a.Cur.Counter, a.Cur.Issued); err != nil {
		return err
	}
	return atomicWrite(d, ActiveFile, encodeActive(Active{Cur: *a.Prev}))
}

// Boot returns the running image after checking that its slot matches the digest in
// the active file. If it does not, fall back with Rollback and return the previous
// image. No active file, or no verifiable image, is ErrNoImage.
func Boot(d *Disk) (Image, error) {
	a, ok, err := readActive(d)
	if err != nil {
		return Image{}, err
	}
	if !ok {
		return Image{}, ErrNoImage
	}
	if body, _ := d.Read(a.Cur.Slot); digestOf(body) == a.Cur.Digest {
		return Image{a.Cur.Slot, a.Cur.Version, a.Cur.Counter, body}, nil
	}
	if a.Prev == nil {
		return Image{}, ErrNoImage
	}
	if err := Rollback(d); err != nil {
		return Image{}, err
	}
	body, _ := d.Read(a.Prev.Slot)
	return Image{a.Prev.Slot, a.Prev.Version, a.Prev.Counter, body}, nil
}

// Install runs the whole update: Recover, LoadState, Verify, write the spare slot
// and read it back (ErrWriteVerify if it differs), commit by atomically replacing
// active (prev = the old cur), raise the high-water mark, then run cfg.Health on the
// installed body; if unhealthy, Rollback and return ErrUnhealthy. A package that
// fails Verify must not cause a single mutating disk operation.
func Install(d *Disk, cfg Config, pkg Package, now int64) error {
	if err := Recover(d); err != nil {
		return err
	}
	st, err := LoadState(d)
	if err != nil {
		return err
	}
	m, err := Verify(cfg, st, pkg, now)
	if err != nil {
		return err
	}
	old, hasOld, err := readActive(d)
	if err != nil {
		return err
	}
	target := SlotA
	if hasOld {
		target = otherSlot(old.Cur.Slot)
	}
	if err := d.Write(target, pkg.Body); err != nil {
		return err
	}
	if back, _ := d.Read(target); !bytes.Equal(back, pkg.Body) {
		return ErrWriteVerify
	}
	next := Active{Cur: Record{target, m.Counter, m.Issued, m.Version, m.Digest}}
	if hasOld {
		prev := old.Cur
		next.Prev = &prev
	}
	if err := atomicWrite(d, ActiveFile, encodeActive(next)); err != nil {
		return err
	}
	if err := raiseHW(d, m.Counter, m.Issued); err != nil {
		return err
	}
	if cfg.Health != nil && !cfg.Health(pkg.Body) {
		if err := Rollback(d); err != nil {
			return fmt.Errorf("%w (rollback failed: %v)", ErrUnhealthy, err)
		}
		return ErrUnhealthy
	}
	return nil
}

// ------------------------------------------------------------ demo

func makePackage(priv ed25519.PrivateKey, version string, counter uint64, issued, expires int64, body []byte) Package {
	meta := []byte(fmt.Sprintf("version=%s\ncounter=%d\nissued=%d\nexpires=%d\nsize=%d\ndigest=%s\n", version, counter, issued, expires, len(body), digestOf(body)))
	return Package{meta, ed25519.Sign(priv, meta), body}
}

func main() {
	seed := make([]byte, ed25519.SeedSize)
	copy(seed, "lesson8-demo-signing-seed")
	priv := ed25519.NewKeyFromSeed(seed)
	cfg := Config{Pub: priv.Public().(ed25519.PublicKey)}
	d := NewDisk()

	fmt.Println("install 1.0:", Install(d, cfg, makePackage(priv, "1.0", 1, 0, 1000, []byte("toy firmware 1.0")), 10))
	// The disk fails halfway through writing 1.1's spare slot, and the process dies.
	d.Plan = map[int]Fault{d.Ops + 1: FaultTornCrash}
	fmt.Println("install 1.1:", Install(d, cfg, makePackage(priv, "1.1", 2, 50, 1000, []byte("toy firmware 1.1")), 60))
	d.Plan = nil
	d.Restart()
	img, err := Boot(d)
	fmt.Println("after crash boots:", string(img.Body), err)
	fmt.Println("retry 1.1:", Install(d, cfg, makePackage(priv, "1.1", 2, 50, 1000, []byte("toy firmware 1.1")), 70))
	img, err = Boot(d)
	fmt.Println("now boots:", string(img.Body), err)
}
