Read the whole input into arrays first, because the first-ok answer needs events that come after the alert.
===
Walk the events in order. Skip everything that is not `fail`, and skip allowlisted sources. For each remaining event count failures from the same source with `ts > t - W`, and alert when the count reaches `N` and the source is not suppressed.
===
Remember suppression is per source: store a "quiet until `t + W`" time for each source that has alerted. For `first-ok`, scan forward from the alert event for the first `ok` whose source is the same one, and print `none` if the scan finds nothing.
===
If alerts show up for allowlisted hosts, you apply the allowlist only to the printing and not to the counting loop. If `first-ok` shows a time for the wrong host, your forward scan forgot to compare the source. The forward scan on a different example (first `reboot` of a given host after index `i`):
```c
long when = -1;
for (int j = i; j < n; j++) if (!strcmp(kind[j], "reboot") && !strcmp(host[j], h)) { when = ts[j]; break; }
```
