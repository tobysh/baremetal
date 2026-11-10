First sort all events by timestamp (keeping input order for ties), because the input is not in time order. Only after sorting can you tell whether two events of one user are close together.
===
Walk the sorted events and keep, for each user, the burst they are currently in. If the user has no burst, or `t - last_time_of_that_burst > G`, start a new burst; otherwise extend it by updating `last`, `n` and the host list (add a host only if it is new).
===
If a burst that should split stays together, you probably compare with the burst start instead of the previous event, or use `>=` where `>` is needed. If bursts of different users interleave in the wrong order, sort the finished bursts by first time and then by user with `strcmp`. If hosts appear twice, check for an existing entry before adding.
===
Starting a new group when a gap is too large, on a different example (heartbeats per sensor, sorted by time):
```c
if (cur < 0 || t - last_seen[cur] > MAXGAP) cur = new_group(sensor, t);
last_seen[cur] = t;
```
