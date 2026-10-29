Implement `int audit_host(const HostApi *h, int lo, int hi, const int *allowed, int na, const Advisory *adv, int nadv, Finding *out, int cap)`.

- Probe every port in `lo..hi` ascending with `h->probe(h->ctx, port)`; retry `PORT_TIMEOUT` at most twice (3 attempts), then treat the port as not open. Never re-probe after a definite answer.
- For each open port call `h->banner(h->ctx, port)` once (it may return `NULL`) and add findings as described in the lesson.
- Return how many findings were written (stop writing at `cap`, but still return the number written). Return `-1` for an invalid range (`lo < 1`, `hi > 65535`, `lo > hi`), without probing.

`ver_cmp` is provided and correct; leave it as it is. Different hosts are generated for every test.
