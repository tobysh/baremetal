import sys, os, re
here = os.path.dirname(os.path.abspath(__file__))
master = open(os.path.join(here, "master.rs")).read().split("\n")
def cond(spec, L):
    m = re.match(r"(>=|<|==)(\d+)", spec); op, n = m.group(1), int(m.group(2))
    return L >= n if op == ">=" else (L < n if op == "<" else L == n)
def build(L, starter):
    out = []; stack = []  # (active_parent, cond_val, in_else)
    task = None  # [N, part]
    def active():
        return all((c != e) if False else (c if not e else not c) for (c, e) in stack)
    for line in master:
        s = line.strip()
        if s.startswith("//@if "):
            stack.append((cond(s[6:].strip(), L), False)); continue
        if s == "//@else":
            c, _ = stack.pop(); stack.append((c, True)); continue
        if s == "//@endif":
            stack.pop(); continue
        if s.startswith("//@task "):
            task = [int(s.split()[1]), "head"]; continue
        if s == "//@stub": task[1] = "stub"; continue
        if s == "//@sol": task[1] = "sol"; continue
        if s == "//@foot": task[1] = "foot"; continue
        if s == "//@endtask": task = None; continue
        if not active(): continue
        if task:
            N, part = task
            if L < N: continue
            if part == "stub" and not (L == N and starter): continue
            if part == "sol" and (L == N and starter): continue
        out.append(line)
    return "\n".join(out)
for L in range(1, 15):
    d = [x for x in os.listdir(os.path.dirname(here)) if x.startswith("%02d-" % L)][0]
    base = os.path.join(os.path.dirname(here), d)
    mainf = open(os.path.join(here, "main_%d.rs" % L)).read()
    for starter, name in ((True, "starter.rs"), (False, "solution.rs")):
        body = build(L, starter)
        body = re.sub(r"\n{3,}", "\n\n", body).rstrip() + "\n\n" + mainf
        open(os.path.join(base, name), "w").write(body)
