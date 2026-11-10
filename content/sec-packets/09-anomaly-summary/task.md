Keep the supplied `Flow` struct. Implement `size_t scan_sources(const Flow *f, size_t n, size_t threshold, uint32_t *out, size_t max)`. It reports the sources whose SYN activity reaches `threshold` distinct destination ports.

**Qualifying flow:** a flow with `(flags & 0x12) == 0x02` (SYN set, ACK clear). Other flag bits do not matter. SYN-ACK (0x12), plain ACK (0x10), RST and flows with no SYN never count.

**Counting:** for each distinct `src`, count the distinct `port` values among its qualifying flows. Repeating a port, or sending it from a different flag combination that still qualifies, adds nothing. A port seen only in non-qualifying flows is not counted. The same port used by two different sources counts once for each.

**Output:** every source whose count is **at least** `threshold` is stored once in `out`, in ascending numeric (`uint32_t`) order. At most `max` values are written, and when there are more than `max` the lowest addresses are kept. Return the number stored. Do not write beyond `out[max - 1]`.

**Edge cases:** return 0 when `threshold == 0` (the rule is disabled), `n == 0` or `max == 0`; in those cases `out` and `f` may be NULL and nothing is written. You may assume `n <= 128`.

The tests include generated flow lists with random sources, ports and flags, every threshold from 1 up, addresses above 128.0.0.0, repeated flows and several values of `max`. Keep the supplied function name and types.
