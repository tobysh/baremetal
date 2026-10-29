Read stdin (at most 1000 lines; ignore malformed lines). Print:

- For each source with at least 5 `FAIL` events, sorted by failures descending then `strcmp(source)`: `BRUTE SRC fails=N`, and if there is an `OK` from that source after its fifth failure, append ` compromised user=U host=H at=TS` for the first such `OK`. If there are no guessing sources, print `BRUTE none`.
- Then one line `PIVOTS N`: the number of `OK` events that have a parent (an earlier `OK` whose `DST` equals this event's `SRC`, with `e.TS - p.TS <= 600`).

Different logs are tried, including sources that succeed before their fifth failure.
