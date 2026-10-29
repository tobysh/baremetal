Read stdin (at most 1000 events, 50 sources; malformed lines ignored). The message is everything after the source, and its first word is the event code. Print, in this order:

1. `TIMELINE`, then every event as `TRUE_TS SOURCE message`, sorted by true time; events with equal true time keep their input order.
2. `PHASES`, then for each phase that has events, in the order recon, access, persistence, lateral, exfil: `PHASE name first=TS last=TS events=N` (true times).
3. `DWELL N`: the last true time minus the first true time among events that belong to a phase; `0` if there are none.
4. `SEQUENCE in-order` if the `first` times of the present phases never decrease in the order above, otherwise `SEQUENCE out-of-order`.

Several incidents with different clock offsets are tried.
