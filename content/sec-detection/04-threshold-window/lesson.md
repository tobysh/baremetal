---
title: Alert on a threshold in a time window
summary: Count events per key in a sliding window, alert when the threshold is reached and suppress repeats for one window.
minutes: 35
difficulty: medium
tags: security, detection, windows
id: sec-detection/threshold-window
mode: stdout
---
Many attacks are visible only as a rate: five failed logins in a minute, twenty denied connections in ten seconds. A threshold rule says "N or more events for the same key within W seconds". Counting all events ever seen would alert on any busy account eventually; a window counts only the recent ones. The window is what separates a typo from a password-guessing run.

## Defining the window exactly

Off-by-one errors here decide whether a rule fires, so write the definition down. When an event arrives at time `t`, the window holds events of the same key with timestamp strictly greater than `t - W`, up to and including `t`. Two events exactly `W` seconds apart are therefore not in the same window. The current event counts itself.

Once a rule fires, it should not fire again on every following event. Suppress repeats for one window per key: after an alert at `t`, the key stays quiet until `t + W`, and an event at exactly `t + W` may alert again. This keeps analysts from drowning in duplicates, which links to [tuning](/learn/sec-detection/false-positive-tuning) later.

## Worked example

Parameters `W=10 N=3`, events as `<time> <key>`:

```text
100 10.0.0.1
104 10.0.0.1
106 10.0.0.2
109 10.0.0.1      count for .1 = 3 (100,104,109 all > 99)  -> ALERT
110 10.0.0.1      count = 3 (104,109,110), but suppressed until 119
110 10.0.0.2
119 10.0.0.1      count = 2 (110,119): window (109,119] excludes 109
```

```text
109 10.0.0.1 count=3
```

The event at 110 would be a second alert without suppression, and the event at 100 has already left the window when 110 arrives (`100 > 100` is false). A window that used `>=` would have counted it.

## Pitfalls

- **Strict versus inclusive.** The lower bound is exclusive (`ts > t - W`). Check the case of two events exactly `W` apart.
- **Per key.** Counts and suppression are kept per key, so a noisy `10.0.0.1` never hides `10.0.0.2`.
- **Same second.** Several events can share a timestamp; each one is processed in input order and counts all earlier ones.
- **Suppression versus window.** Suppression ends at `t + W` with an inclusive comparison, and it is separate from the count: a suppressed event still counts toward later windows.
- **Printing the count.** `count=` shows the window count at the alert, which can exceed `N`.

## Why defenders care

The threshold and window are the tuning knobs of a rate rule. Too low and a user who forgot their password pages someone; too high and a slow guesser stays under the radar. Writing the definition down makes the trade-off a decision rather than an accident. You met counting per source in [authentication log analysis](/learn/sec-forensics/auth-log-analysis); this time order and time matter.
