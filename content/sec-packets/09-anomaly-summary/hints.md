Qualify a flow with `(flags & 0x12) == 0x02`. Write that once as a small helper so every loop uses the same test.
===
Process each distinct source once: skip a flow if an earlier flow has the same `src`. For that source, scan all qualifying flows and count a port only on its first qualifying appearance (no earlier flow with the same `src`, same `port` and a qualifying flag).
===
Keep the sources that reach `threshold` in a local array of up to 128 values. When the scan is done, sort that array numerically (an insertion sort is plenty for 128 entries).
===
Apply `max` after sorting: copy `min(count, max)` values to `out` and return that number. Return 0 at the start for `threshold == 0`, `n == 0` or `max == 0`, before touching any pointer.
