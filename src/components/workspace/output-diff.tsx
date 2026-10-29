/** Literal comparison of expected and actual output, with whitespace made visible. */

const norm = (s: string) => s.replace(/\r\n/g, "\n").replace(/[ \t]+$/gm, "").trimEnd().split("\n");

/** Tabs and spaces are drawn as glyphs so "looks the same" differences show up. */
const visible = (s: string) => s.replace(/\t/g, "→").replace(/ /g, "·");

function split(a: string, b: string) {
  let start = 0;
  while (start < a.length && start < b.length && a[start] === b[start]) start++;
  let endA = a.length;
  let endB = b.length;
  while (endA > start && endB > start && a[endA - 1] === b[endB - 1]) {
    endA--;
    endB--;
  }
  return { a: [a.slice(0, start), a.slice(start, endA), a.slice(endA)], b: [b.slice(0, start), b.slice(start, endB), b.slice(endB)] };
}

function Line({ n, parts, bad, missing }: { n: number; parts: string[] | null; bad: boolean; missing: string }) {
  return (
    <div className={`flex gap-2 ${bad ? "bg-err-soft" : ""}`}>
      <span aria-hidden="true" className="w-5 shrink-0 text-right opacity-60 select-none">{n}</span>
      {parts ? (
        <span className="whitespace-pre-wrap break-all">
          {visible(parts[0])}
          {parts[1] && <mark className="rounded-sm bg-err px-px text-err-ink">{visible(parts[1])}</mark>}
          {visible(parts[2])}
          {!parts[0] && !parts[1] && !parts[2] && <span className="opacity-60">(empty line)</span>}
        </span>
      ) : (
        <span className="text-err italic">{missing}</span>
      )}
    </div>
  );
}

export function OutputDiff({ expected, actual }: { expected: string; actual: string }) {
  const exp = norm(expected);
  const act = actual.trim() === "" ? [] : norm(actual);
  const rows = Math.max(exp.length, act.length);
  const first = Array.from({ length: rows }).findIndex((_, i) => exp[i] !== act[i]);
  const pairs = Array.from({ length: rows }, (_, i) => {
    const e = exp[i];
    const a = act[i];
    return e !== undefined && a !== undefined && e !== a ? split(e, a) : null;
  });
  return (
    <div className="mt-1 ml-5 space-y-2 text-xs">
      <p className="text-muted">
        {act.length === 0 ? "Your program printed nothing." : `First difference is on line ${first + 1}.`} In the grids below <code className="font-mono">→</code> is a tab and <code className="font-mono">·</code> is a space.
      </p>
      <div className="grid gap-2 sm:grid-cols-2">
        {([["Expected", exp, "expected"], ["Your output", act, "yours"]] as const).map(([label, lines, kind]) => (
          <div key={label}>
            <div className="mb-1 font-medium text-muted">{label}</div>
            <div className="overflow-auto rounded-md bg-bg p-2 font-mono text-muted">
              {Array.from({ length: rows }, (_, i) => {
                const line = lines[i];
                const pair = pairs[i];
                const bad = exp[i] !== act[i];
                const parts = line === undefined ? null : pair ? (kind === "expected" ? pair.a : pair.b) : [line, "", ""];
                return <Line key={i} n={i + 1} parts={parts} bad={bad} missing={kind === "yours" ? "(missing line)" : "(nothing expected here)"} />;
              })}
            </div>
          </div>
        ))}
      </div>
    </div>
  );
}
