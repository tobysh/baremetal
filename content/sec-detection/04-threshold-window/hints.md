The rule is about one key at a time. For each arriving event, ask "how many events with this same key are inside the window right now?" and only then decide whether to alert.
===
Keep all events seen so far. For each new event scan back over earlier events with the same key and count those whose time is still inside the window, which means `ts > t - W`, with strict greater-than. The event itself counts.
===
If two events exactly `W` apart are counted together, your lower bound is inclusive. If you get a burst of alerts on consecutive events, you forgot suppression; if alerts vanish for too long, your suppress-until comparison is off by one: an event at exactly `t + W` may alert again, so compare with `t >= suppress_until`.
===
Counting in a window, on a different example (login attempts, window `W`, current time `now`):
```c
int c = 0;
for (int i = n - 1; i >= 0; i--)
    if (strcmp(who[i], user) == 0 && when[i] > now - W) c++;
```
