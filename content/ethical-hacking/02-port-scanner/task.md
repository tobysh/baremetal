**`scan_ports(probe, ctx, lo, hi, open, cap, filtered)`** probes every port from `lo` to `hi` inclusive, in ascending order, through `probe(ctx, port)`.

- A port answering `PORT_OPEN` is stored in `open[]` (ascending, at most `cap` entries; extra open ports are still probed but not stored). Return the number stored.
- A `PORT_TIMEOUT` is retried, at most twice. If the third attempt also times out, count the port in `*filtered`. A port that answers on any attempt is not filtered.
- Never probe a port again after it gave a definite answer (open or closed). Never probe outside `lo..hi`.
- If `lo < 1`, `hi > 65535` or `lo > hi`, return `-1` without probing and without touching `*filtered`.
- `*filtered` starts at 0 for each call.

**`unexpected_open(open, n, allowed, na, out)`** copies into `out[]` every port in `open[0..n-1]` that is not in `allowed[0..na-1]`, keeping order, and returns how many.

Different simulated hosts are used, including flaky links.
