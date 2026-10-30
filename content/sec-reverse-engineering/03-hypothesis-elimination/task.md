Input: a line with `N`, then `N` lines `in out` (integers, the observations in order), then a line with `M`, then `M` hypothesis names, one per line.

For each hypothesis name, in input order, print one line:

- `<name>: consistent` if it reproduces every observation.
- `<name>: refuted by #<k> (in=<i> expected=<o> got=<g>)` for the **first** observation (numbered from 1) it gets wrong; `expected` is the observed output and `got` is what the hypothesis computes.
- `<name>: unknown hypothesis` if the name is not in the menu.

Then print exactly one verdict line, based on the consistent hypotheses only:

- none: `verdict: no hypothesis fits`
- exactly one: `verdict: <name> fits all observations (not proven)`
- several: `verdict: ambiguous (<a>, <b>, ...) - more observations needed`, in input order.

With zero observations every known hypothesis is consistent.
