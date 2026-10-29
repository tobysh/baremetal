#!/usr/bin/env python3
"""Generates the SYNTHETIC evidence for content/sec-forensics/.

Deterministic (fixed seeds): running it twice gives byte-identical output.
It writes, per lesson, either tests.c (hidden harness with the evidence embedded as C arrays,
from the templates in scripts/content-gen/sec-forensics/<slug>.c) or cases.json (stdin cases).
The expected answers come from the independent Python oracle below, not from the lesson solutions.

Usage: python3 scripts/content-gen/sec-forensics.py
Everything is fictional: example.test domains, documentation IP ranges, made-up names.
"""
import datetime, json, pathlib, random, struct

ROOT = pathlib.Path(__file__).resolve().parents[2] / "content" / "sec-forensics"
TPL = pathlib.Path(__file__).resolve().parent / "sec-forensics"

def arr(name, b):
    b = bytes(b) or b"\0"
    lines = []
    for i in range(0, len(b), 16):
        lines.append("    " + ", ".join("0x%02x" % x for x in b[i:i + 16]) + ",")
    return "static const uint8_t %s[] = {\n%s\n};\n" % (name, "\n".join(lines))

def cstr(s):
    if isinstance(s, bytes):
        s = s.decode("latin-1")
    out = []
    for ch in s:
        o = ord(ch)
        if ch == "\n": out.append("\\n")
        elif ch == '"': out.append('\\"')
        elif ch == "\\": out.append("\\\\")
        elif ch == "?": out.append("\\?")
        elif 32 <= o < 127: out.append(ch)
        else: out.append("\\%03o" % o)
    return '"' + "".join(out) + '"'

def write_tests(slug, data):
    tpl = (TPL / (slug + ".c")).read_text()
    assert "//@DATA@" in tpl
    d = ROOT / slug
    d.mkdir(parents=True, exist_ok=True)
    (d / "tests.c").write_text(tpl.replace("//@DATA@", data))

def write_cases(slug, cases):
    d = ROOT / slug
    d.mkdir(parents=True, exist_ok=True)
    (d / "cases.json").write_text(json.dumps(cases, indent=2) + "\n")

def junk(r, n, avoid=()):
    ok = [x for x in range(256) if x not in avoid]
    return bytes(r.choice(ok) for _ in range(n))

# ---------------------------------------------------------------- 01 magic bytes
SIGS = [
    ("PNG", b"\x89PNG\r\n\x1a\n"), ("JPEG", b"\xff\xd8\xff"), ("GIF", b"GIF87a"), ("GIF", b"GIF89a"),
    ("PDF", b"%PDF-"), ("ZIP", b"PK\x03\x04"), ("ELF", b"\x7fELF"),
]
def identify(b):
    for name, s in SIGS:
        if b[:len(s)] == s:
            return name
    return "unknown"

def gen_magic():
    r = random.Random(1001)
    samples = []
    for name, s in SIGS:
        samples.append(s + junk(r, r.randrange(4, 60)))
    samples.append(b"\x00" + SIGS[0][1] + junk(r, 10))           # signature not at offset 0
    samples.append(SIGS[0][1][:5])                              # truncated PNG signature
    samples.append(b"")                                         # empty
    samples.append(b"GIF88a" + junk(r, 8))                      # near miss
    samples.append(b"%pdf-1.4" + junk(r, 8))                    # wrong case
    samples.append(junk(r, 40, avoid=(0x89, 0xff, 0x7f, 0x47, 0x25, 0x50)))  # random noise
    samples.append(b"PK\x05\x06" + b"\0" * 18)                  # empty zip: different signature
    samples.append(b"\xff\xd8\xff")                             # exactly the signature
    r.shuffle(samples)
    d, rows = "", []
    for i, s in enumerate(samples):
        d += arr("m%d" % i, s)
        rows.append('    {m%d, %d, "%s"},' % (i, len(s), identify(s)))
    d += "static const struct { const uint8_t *p; size_t n; const char *want; } M[] = {\n%s\n};\n" % "\n".join(rows)
    write_tests("01-file-signatures", d)

# ---------------------------------------------------------------- 02 hexdump
def xxd(b):
    out = ""
    for off in range(0, len(b), 16):
        row = b[off:off + 16]
        groups = [row[i:i + 2].hex() for i in range(0, len(row), 2)]
        hexpart = " ".join(groups).ljust(39)
        asc = "".join(chr(c) if 32 <= c < 127 else "." for c in row)
        out += "%08x: %s  %s\n" % (off, hexpart, asc)
    return out

def gen_hexdump():
    r = random.Random(1002)
    samples = [b"", b"A", b"Hello, evidence!\n", b"0123456789abcdef", b"0123456789abcdefg"]
    for n in (14, 33, 48, 70):
        samples.append(bytes(r.randrange(256) for _ in range(n)))
    samples.append(bytes(range(0x7c, 0x7c + 20)))  # crosses the printable boundary
    samples.append(b"\x00\x1f\x20\x7e\x7f\x80\xff" * 5)
    d, rows = "", []
    for i, s in enumerate(samples):
        d += arr("h%d" % i, s)
        rows.append("    {h%d, %d, %s}," % (i, len(s), cstr(xxd(s))))
    d += "static const struct { const uint8_t *p; size_t n; const char *want; } H[] = {\n%s\n};\n" % "\n".join(rows)
    write_tests("02-hex-dump", d)

# ---------------------------------------------------------------- 03 carving
def make_jpeg(r):
    body = bytearray()
    while len(body) < r.randrange(40, 200):
        if r.random() < 0.05: body += b"\xff\x00"          # byte stuffing, not a footer
        else: body += junk(r, 1, avoid=(0xff,))
    return b"\xff\xd8\xff\xe0" + bytes(body) + b"\xff\xd9"

def make_png(r):
    body = junk(r, r.randrange(30, 160), avoid=(0xff, 0x89, 0x49))
    return b"\x89PNG\r\n\x1a\n" + b"\0\0\0\rIHDR" + body + b"\0\0\0\0IEND" + junk(r, 4, avoid=(0xff,))

def carve(img):
    res, i = [], 0
    while i < len(img):
        if img[i:i + 3] == b"\xff\xd8\xff":
            e = img.find(b"\xff\xd9", i + 3)
            if e >= 0:
                res.append(("JPEG", i, e + 2 - i)); i = e + 2; continue
        elif img[i:i + 8] == b"\x89PNG\r\n\x1a\n":
            e = img.find(b"IEND", i + 8)
            if e >= 0 and e + 8 <= len(img):
                res.append(("PNG", i, e + 8 - i)); i = e + 8; continue
        i += 1
    return res

def gen_carve():
    r = random.Random(1003)
    imgs = []
    for k in range(4):
        pieces = []
        for _ in range(r.randrange(2, 5)):
            pieces.append(r.choice([make_jpeg, make_png])(r))
        img = b""
        for p in pieces:
            img += junk(r, r.randrange(3, 150), avoid=(0xff, 0x89))
            img += p
        img += junk(r, r.randrange(1, 90), avoid=(0xff, 0x89))
        if k == 2:  # a truncated JPEG at the very end: header, no footer
            img += b"\xff\xd8\xff\xe1" + junk(r, 60, avoid=(0xff,))
        if k == 3:  # a file starting at offset 0
            img = pieces[0] + img
        imgs.append(img)
    d, rows = "", []
    for i, img in enumerate(imgs):
        d += arr("img%d" % i, img)
        exp = carve(img)
        ex = ", ".join('{"%s", %d, %d}' % e for e in exp)
        rows.append("    {img%d, %d, %d, {%s}}," % (i, len(img), len(exp), ex))
    d += "typedef struct { const char *type; size_t offset, length; } Want;\n"
    d += "static const struct { const uint8_t *p; size_t n; int count; Want want[8]; } I[] = {\n%s\n};\n" % "\n".join(rows)
    write_tests("03-carving-files", d)

# ---------------------------------------------------------------- 04 MBR
TYPES = {0x07: "NTFS/exFAT", 0x0b: "FAT32 (CHS)", 0x0c: "FAT32 (LBA)", 0x82: "Linux swap", 0x83: "Linux",
         0x05: "Extended", 0xee: "GPT protective", 0xef: "EFI System"}

def make_mbr(r, slots, sig=b"\x55\xaa"):
    s = bytearray(junk(r, 446))
    t = b""
    for e in slots:
        if e is None: t += b"\0" * 16
        else:
            boot, typ, lba, n = e
            t += struct.pack("<B3sB3sII", 0x80 if boot else 0, b"\xfe\xff\xff", typ, b"\xfe\xff\xff", lba, n)
    return bytes(s) + t + sig

def gen_mbr():
    r = random.Random(1004)
    layouts = [
        [(True, 0x07, 2048, 409600), (False, 0x83, 411648, 1048576), None, None],
        [None, None, (False, 0x0c, 63, 8000000), (True, 0x82, 0xF0000000, 0x0FFFFFF0)],
        [(True, 0xee, 1, 0xFFFFFFFF), None, None, None],
        [(False, 0x0b, 2048, 100), (False, 0x05, 4096, 9000), (False, 0xef, 20000, 65536), (False, 0x99, 99999, 5)],
        [None, None, None, None],
    ]
    mbrs = [make_mbr(r, l) for l in layouts]
    mbrs.append(make_mbr(r, layouts[0], sig=b"\xaa\x55"))   # bad signature (swapped)
    d, rows = "", []
    for i, (m, l) in enumerate(zip(mbrs, layouts + [layouts[0]])):
        d += arr("mbr%d" % i, m)
        used = [e for e in l if e]
        ok = m[510:512] == b"\x55\xaa"
        ex = ", ".join("{%d, 0x%02x, %du, %du}" % (1 if b else 0, t, lba, n) for b, t, lba, n in used)
        rows.append("    {mbr%d, %d, {%s}}," % (i, len(used) if ok else -1, ex))
    d += "static const struct { const uint8_t *p; int count; Partition want[4]; } B[] = {\n%s\n};\n" % "\n".join(rows)
    names = ", ".join("{0x%02x, %s}" % (k, cstr(v)) for k, v in TYPES.items())
    d += "static const struct { uint8_t type; const char *name; } N[] = { %s, {0x00, \"Unknown\"}, {0xa5, \"Unknown\"} };\n" % names
    write_tests("04-partition-tables", d)

# ---------------------------------------------------------------- 05 FAT directory
NAMES = ["REPORT", "PHOTO001", "NOTES", "INVOICE", "BACKUP", "ALIBI", "LEDGER", "MEMO", "TRIP", "CONTACTS", "SECRET", "TAXES"]
EXTS = ["TXT", "JPG", "DOC", "ZIP", "PDF", "   ", "XLS"]

def fat_entry(name83, attr, cluster, size):
    return struct.pack("<11sBBBHHHHHHHI", name83, attr, 0, 0, 0, 0, 0, cluster >> 16, 0, 0, cluster & 0xffff, size)

def gen_fat():
    r = random.Random(1005)
    dirs = []
    for k in range(4):
        ents, want = [], []
        if k != 1:
            ents.append(fat_entry(b"EXAMPLE_VOL"[:11].ljust(11), 0x08, 0, 0))   # volume label, not a file
        for _ in range(r.randrange(5, 9)):
            base, ext = r.choice(NAMES), r.choice(EXTS)
            deleted = r.random() < 0.4
            is_dir = (not deleted) and r.random() < 0.15
            if is_dir: ext = "   "
            n83 = (base.ljust(8) + ext).encode()
            if r.random() < 0.3:   # a long-file-name fragment precedes the entry
                lfn = bytearray(junk(r, 32, avoid=(0,))); lfn[0] = 0x41; lfn[11] = 0x0f
                ents.append(bytes(lfn))
            cl = r.randrange(2, 0x1fffff)
            sz = 0 if is_dir else r.choice([r.randrange(0, 5000), r.randrange(0, 2**32)])
            first = n83[0:1]
            if deleted: n83 = b"\xe5" + n83[1:]
            ents.append(fat_entry(n83, 0x10 if is_dir else 0x20, cl, sz))
            shown = (base[:8].rstrip() if not deleted else "_" + base[1:8].rstrip())
            nm = shown + ("." + ext.strip() if ext.strip() else "")
            want.append((nm, deleted, is_dir, sz, cl))
        raw = b"".join(ents) + b"\0" * 32
        raw += fat_entry(b"AFTEREND TXT", 0x20, 9, 9) * 2        # junk after the end marker
        dirs.append((raw, want))
    d, rows = "", []
    for i, (raw, want) in enumerate(dirs):
        d += arr("dir%d" % i, raw)
        ex = ", ".join("{%s, %d, %d, %du, %du}" % (cstr(n), de, di, sz, cl) for n, de, di, sz, cl in want)
        rows.append("    {dir%d, %d, %d, {%s}}," % (i, len(raw), len(want), ex))
    d += "static const struct { const uint8_t *p; size_t n; int count; DirEntry want[16]; } D[] = {\n%s\n};\n" % "\n".join(rows)
    write_tests("05-fat-directory", d)

# ---------------------------------------------------------------- 06 hash audit + 07 review
def fnv(b):
    h = 0xcbf29ce484222325
    for c in b:
        h = ((h ^ c) * 0x100000001b3) & 0xffffffffffffffff
    return h

def gen_hash():
    r = random.Random(1006)
    d = ""
    scen = []
    for k in range(3):
        items = []
        for j in range(r.randrange(3, 6)):
            nm = "%s_%02d.%s" % (r.choice(["disk", "mail", "log", "photo", "memo"]), r.randrange(1, 90), r.choice(["img", "pst", "txt", "jpg"]))
            items.append([nm, bytes(r.randrange(256) for _ in range(r.randrange(0, 120)))])
        names = {i[0] for i in items}
        assert len(names) == len(items)
        manifest = [(fnv(b), n) for n, b in items]
        status = [0] * len(items)
        if k >= 1:
            t = r.randrange(len(items))
            if items[t][1]: items[t][1] = bytes([items[t][1][0] ^ 1]) + items[t][1][1:]
            else: items[t][1] = b"x"
            status[t] = 1
        if k == 2:
            items.append(["extra_%02d.bin" % r.randrange(90), b"planted\n"]); status.append(2)
            manifest.append((fnv(b"gone"), "missing_file.bin"))   # in the manifest, not in the evidence
        r.shuffle(manifest)
        mtext = "".join("%016x  %s\n" % m for m in manifest)
        scen.append((items, mtext, status))
    rows = []
    for k, (items, mtext, status) in enumerate(scen):
        ir = []
        for j, (n, b) in enumerate(items):
            d += arr("f%d_%d" % (k, j), b)
            ir.append('{"%s", f%d_%d, %d}' % (n, k, j, len(b)))
        rows.append("    {%s, {%s}, %d, {%s}}," % (cstr(mtext), ", ".join(ir), len(items), ", ".join(map(str, status))))
    d += "static const struct { const char *manifest; Item items[8]; int n; int want[8]; } A[] = {\n%s\n};\n" % "\n".join(rows)
    vec = [b"", b"a", b"foobar", bytes(range(256)), b"The quick brown fox jumps over the lazy dog"]
    vr = []
    for i, v in enumerate(vec):
        d += arr("v%d" % i, v)
        vr.append("    {v%d, %d, 0x%016xULL}," % (i, len(v), fnv(v)))
    d += "static const struct { const uint8_t *p; size_t n; uint64_t want; } V[] = {\n%s\n};\n" % "\n".join(vr)
    write_tests("06-hash-verification", d)

REV = [("PNG", b"\x89PNG\r\n\x1a\n"), ("JPEG", b"\xff\xd8\xff"), ("PDF", b"%PDF-"), ("ZIP", b"PK\x03\x04")]
def rkind(b):
    for n, s in REV:
        if b[:len(s)] == s: return n
    return "unknown"

def gen_review():
    r = random.Random(1007)
    d, rows = "", []
    cases = []
    for i in range(7):
        head = r.choice(REV + [("x", b"\x00\x01")])[1]
        b = head + bytes(r.randrange(256) for _ in range(r.randrange(0, 40)))
        good = fnv(b)
        exp_hash = good if r.random() < 0.5 else good ^ (1 << r.randrange(64))
        cases.append(("item%d.bin" % (i + 1), b, exp_hash))
    cases.append(("tiny.bin", b"\xff", fnv(b"\xff")))
    cases.append(("empty.bin", b"", fnv(b"")))
    for i, (n, b, h) in enumerate(cases):
        first = " ".join("%02x" % c for c in b[:8]) or "-"
        line = "%s: %s, first bytes %s, hash %016x, %s\n" % (n, rkind(b), first, fnv(b), "OK" if h == fnv(b) else "CHANGED")
        d += arr("e%d" % i, b)
        rows.append('    {"%s", e%d, %d, 0x%016xULL, %s},' % (n, i, len(b), h, cstr(line)))
    d += "static const struct { const char *name; const uint8_t *p; size_t n; uint64_t expect; const char *want; } R[] = {\n%s\n};\n" % "\n".join(rows)
    write_tests("07-review-evidence-triage", d)

# ---------------------------------------------------------------- 08 EXIF-like (TIFF) metadata
def to_unix(s):
    if len(s) != 19 or s[4] != ":" or s[7] != ":" or s[10] != " " or s[13] != ":" or s[16] != ":":
        return -1
    try:
        y, mo, d, h, mi, se = int(s[0:4]), int(s[5:7]), int(s[8:10]), int(s[11:13]), int(s[14:16]), int(s[17:19])
    except ValueError:
        return -1
    if not (1970 <= y <= 2100): return -1
    try:
        return int(datetime.datetime(y, mo, d, h, mi, se, tzinfo=datetime.timezone.utc).timestamp())
    except ValueError:
        return -1

def tiff(r, big, make, model, w, h, dt, order):
    e = ">" if big else "<"
    tags = {0x010f: ("a", make), 0x0110: ("a", model), 0x0100: ("s" if w < 65536 and r.random() < .5 else "l", w),
            0x0101: ("l", h), 0x0132: ("a", dt)}
    keys = [k for k in order if k in tags]
    n = len(keys)
    pad = r.randrange(0, 5)
    ifd = 8 + pad
    data_at = ifd + 2 + 12 * n + 4
    strings = b""
    ents = b""
    for k in sorted(keys):
        t, v = tags[k]
        if t == "a":
            s = v.encode() + b"\0"
            if len(s) <= 4:
                ents += struct.pack(e + "HHI", k, 2, len(s)) + s.ljust(4, b"\0")
            else:
                ents += struct.pack(e + "HHII", k, 2, len(s), data_at + len(strings))
                strings += s
        elif t == "s": ents += struct.pack(e + "HHI", k, 3, 1) + struct.pack(e + "H", v) + b"\0\0"
        else: ents += struct.pack(e + "HHII", k, 4, 1, v)
    return (b"MM" if big else b"II") + struct.pack(e + "HI", 42, ifd) + b"\xaa" * pad + struct.pack(e + "H", n) + ents + b"\0" * 4 + strings

def gen_exif():
    r = random.Random(1008)
    cams = [("Examplecam", "EC-100"), ("Fictiv", "Pocket 7"), ("Noname", "N1"), ("Testograph", "TG-2 Mk II")]
    full = [0x010f, 0x0110, 0x0100, 0x0101, 0x0132]
    blobs = []
    dts = ["2026:03:02 09:15:00", "2024:02:29 23:59:59", "2025:12:31 00:00:01", "2026:07:14 18:30:45"]
    for k in range(4):
        mk, md = cams[k]
        order = full if k != 3 else [0x0132, 0x0100]
        blobs.append(tiff(r, k % 2 == 1, mk, md, r.choice([640, 1024, 4000, 70000]), r.randrange(100, 3000), dts[k], order))
    # expected parse
    def parse(b):
        if len(b) < 8 or b[:2] not in (b"II", b"MM"): return None
        e = ">" if b[:2] == b"MM" else "<"
        if struct.unpack(e + "H", b[2:4])[0] != 42: return None
        off = struct.unpack(e + "I", b[4:8])[0]
        if off + 2 > len(b): return None
        n = struct.unpack(e + "H", b[off:off + 2])[0]
        if off + 2 + 12 * n > len(b): return None
        m = {"make": "", "model": "", "w": 0, "h": 0, "dt": ""}
        for i in range(n):
            t, ty, c, v = struct.unpack(e + "HHII", b[off + 2 + 12 * i: off + 14 + 12 * i])
            raw = b[off + 2 + 12 * i + 8: off + 14 + 12 * i]
            if ty == 2:
                if not c: return None
                if c <= 4:
                    raw_s = raw[:c]
                else:
                    if v > len(b) or c > len(b) - v: return None
                    raw_s = b[v:v+c]
                if raw_s[-1] != 0: return None
                s = raw_s.split(b"\0", 1)[0].decode()
                key = {0x010f: "make", 0x0110: "model", 0x0132: "dt"}.get(t)
                if key: m[key] = s
            elif ty == 3:
                val = struct.unpack(e + "H", raw[:2])[0]
                if t == 0x0100: m["w"] = val
                if t == 0x0101: m["h"] = val
            elif ty == 4:
                if t == 0x0100: m["w"] = v
                if t == 0x0101: m["h"] = v
        return m
    d, rows = "", []
    cand = list(blobs)
    cand.append(b"XX" + blobs[0][2:])                 # bad byte-order mark
    cand.append(blobs[1][:len(blobs[1]) - 6])         # truncated: last string runs off the end
    cand.append(blobs[0][:6])                         # shorter than the header
    for i, b in enumerate(cand):
        m = parse(b)
        d += arr("x%d" % i, b)
        if m is None:
            rows.append("    {x%d, %d, -1, {\"\", \"\", 0, 0, \"\"}}," % (i, len(b)))
        else:
            rows.append("    {x%d, %d, 0, {%s, %s, %du, %du, %s}}," % (i, len(b), cstr(m["make"]), cstr(m["model"]), m["w"], m["h"], cstr(m["dt"])))
    d += "static const struct { const uint8_t *p; size_t n; int rc; Meta want; } X[] = {\n%s\n};\n" % "\n".join(rows)
    ts = ["2026:03:02 09:15:00", "1970:01:01 00:00:00", "2024:02:29 23:59:59", "2000:01:01 00:00:00", "2100:12:31 23:59:59",
          "2026:13:02 09:15:00", "2026-03-02 09:15:00", "2026:03:02", "abcd:ef:gh ij:kl:mn", "2023:06:15 12:00:00", "1999:12:31 23:59:59"]
    ts += ["2023:02:29 12:00:00", "2100:02:29 00:00:00", "2000:02:29 00:00:00",
           "2026:04:31 00:00:00", "2026:01:00 00:00:00", "2026:01:01 24:00:00",
           "2026:01:01 00:60:00", "2026:01:01 00:00:60", "1969:12:31 23:59:59"]
    d += "static const struct { const char *s; long long want; } T[] = {\n%s\n};\n" % "\n".join("    {%s, %dLL}," % (cstr(s), to_unix(s)) for s in ts)
    write_tests("08-photo-metadata", d)

# ---------------------------------------------------------------- 09 timeline (stdin cases)
def iso(t):
    import datetime
    return datetime.datetime.fromtimestamp(t, datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

def timeline(lines):
    ev = []
    for ln in lines:
        if not ln.strip(): continue
        t, src, msg = ln.split(" ", 2)
        ev.append((int(t), src, msg))
    ev.sort(key=lambda e: e[0])   # python sort is stable
    out, prev = [], None
    for t, src, msg in ev:
        if prev is not None and t - prev >= 3600:
            g = t - prev
            out.append("-- quiet for %dh %02dm --" % (g // 3600, g % 3600 // 60))
        out.append("%s [%s] %s" % (iso(t), src, msg))
        prev = t
    span = ev[-1][0] - ev[0][0]
    out.append("%d events over %dh %02dm" % (len(ev), span // 3600, span % 3600 // 60))
    return "\n".join(out) + "\n"

def gen_timeline():
    r = random.Random(1009)
    msgs = {"mtime": ["modified /home/dana/ledger.ods", "modified /var/www/example.test/upload.php", "created /tmp/.hidden_tool",
                      "modified /etc/cron.d/backup", "modified /home/dana/notes.txt"],
            "auth": ["login ok user=dana from 192.0.2.10", "sudo by dana: cat /etc/shadow", "logout user=dana", "login failed user=root from 203.0.113.9"],
            "web": ["POST /upload.php 200", "GET /index.html 200", "GET /../../etc/passwd 403", "POST /login 302"]}
    cases = []
    spec = [("a few events, one hour of quiet", 6, 1), ("busy morning, no gaps", 9, 0), ("a single event", 1, 0), ("a quiet night then activity", 8, 2)]
    for name, n, gaps in spec:
        base = 1767225600 + r.randrange(0, 200) * 86400 + r.randrange(0, 86400)
        t = base
        ev = []
        for i in range(n):
            t += r.randrange(5, 1500) if not (gaps and i and r.random() < 0.4) else r.randrange(3600, 30000)
            src = r.choice(list(msgs))
            ev.append("%d %s %s" % (t, src, r.choice(msgs[src])))
        if n >= 4:
            ev.append("%d %s %s" % (ev and int(ev[1].split()[0]), "web", "GET /robots.txt 200"))  # tie with an earlier event
        r.shuffle(ev)
        cases.append({"name": name, "stdin": "\n".join(ev) + "\n", "expected": timeline(ev)})
    write_cases("09-building-a-timeline", cases)

# ---------------------------------------------------------------- 10 auth.log brute force
def authlog(lines):
    fails, users, flagged_at, acc = {}, {}, {}, {}
    for idx, ln in enumerate(lines):
        if "sshd[" not in ln: continue
        if " Failed password for " in ln:
            rest = ln.split(" Failed password for ", 1)[1]
            tok = rest.split()
            u = tok[2] if tok[0] == "invalid" else tok[0]
            ip = tok[tok.index("from") + 1]
            fails[ip] = fails.get(ip, 0) + 1
            users.setdefault(ip, set()).add(u)
        elif " Accepted password for " in ln:
            tok = ln.split(" Accepted password for ", 1)[1].split()
            ip = tok[tok.index("from") + 1]
            if fails.get(ip, 0) >= 5 and ip not in acc: acc[ip] = tok[0]
    res = []
    for ip, n in fails.items():
        if n >= 5:
            res.append((-n, ip))
    res.sort()
    out = ""
    for neg, ip in res:
        out += "%s failures=%d users=%d%s\n" % (ip, -neg, len(users[ip]), " ACCEPTED as " + acc[ip] if ip in acc else "")
    return out or "no brute force detected\n"

def gen_auth():
    r = random.Random(1010)
    ips = ["198.51.100.%d" % i for i in (7, 23, 88, 140)] + ["203.0.113.%d" % i for i in (9, 60, 200)] + ["192.0.2.%d" % i for i in (10, 11)]
    guess = ["root", "admin", "test", "oracle", "guest", "deploy", "ubuntu", "carol"]
    clock = [0]
    def stamp():
        clock[0] += r.randrange(1, 40)
        s = clock[0]
        return "Mar  3 %02d:%02d:%02d example-host sshd[%d]: " % (2 + s // 3600, s // 60 % 60, s % 60, 1000 + r.randrange(9000))
    def fail(ip, u):
        inv = "invalid user " if u not in ("root", "alice", "bob", "carol", "deploy") else ""
        return stamp() + "Failed password for %s%s from %s port %d ssh2" % (inv, u, ip, r.randrange(1024, 65535))
    def ok(ip, u): return stamp() + "Accepted password for %s from %s port %d ssh2" % (u, ip, r.randrange(1024, 65535))
    def noise(): return r.choice([
        "Mar  3 03:00:01 example-host CRON[%d]: pam_unix(cron:session): session opened for user root" % r.randrange(9999),
        stamp() + "Connection closed by %s port %d [preauth]" % (r.choice(ips), r.randrange(1024, 65535)),
        stamp() + "Accepted publickey for alice from 192.0.2.10 port %d ssh2" % r.randrange(1024, 65535),
        stamp() + "Failed publickey for bob from 192.0.2.11 port %d ssh2" % r.randrange(1024, 65535)])
    cases = []
    def build(plan, name):
        clock[0] = 0
        lines = []
        for kind, ip, k, who in plan:
            if kind == "fail":
                for _ in range(k):
                    lines.append(fail(ip, who if who else r.choice(guess)))
                    if r.random() < 0.4: lines.append(noise())
            elif kind == "ok": lines.append(ok(ip, who))
            else: lines.append(noise())
        cases.append({"name": name, "stdin": "\n".join(lines) + "\n", "expected": authlog(lines)})
    build([("n", 0, 0, 0), ("fail", ips[0], 4, None), ("ok", "192.0.2.10", 0, "alice"), ("fail", "192.0.2.11", 2, "bob"), ("ok", "192.0.2.11", 0, "bob"), ("n", 0, 0, 0)],
          "no attack: four failures is under the limit")
    build([("fail", ips[1], 5, None), ("fail", ips[2], 11, None), ("ok", "192.0.2.10", 0, "alice"), ("fail", ips[3], 8, "root"), ("n", 0, 0, 0)],
          "three attackers, none got in")
    build([("fail", ips[4], 6, None), ("fail", ips[5], 6, None), ("ok", ips[4], 0, "deploy"), ("fail", "192.0.2.11", 1, "bob"),
           ("ok", ips[6], 0, "alice"), ("fail", ips[6], 3, "alice")], "a ranked tie and one attacker who got in")
    build([("fail", ips[0], 9, "admin"), ("ok", ips[0], 0, "root"), ("fail", ips[0], 3, None), ("ok", ips[0], 0, "carol"),
           ("fail", ips[1], 5, "root"), ("ok", ips[1], 0, "alice")], "success after the fifth failure only counts once")
    build([("fail", ips[2], 4, None), ("ok", ips[2], 0, "alice"), ("fail", ips[2], 1, None), ("ok", ips[2], 0, "bob")], "the fifth failure arrives before the login")
    write_cases("10-auth-log-analysis", cases)

if __name__ == "__main__":
    for g in (gen_magic, gen_hexdump, gen_carve, gen_mbr, gen_fat, gen_hash, gen_review, gen_exif, gen_timeline, gen_auth):
        g()
    print("sec-forensics evidence generated")
