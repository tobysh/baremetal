---
title: Alert on a threshold in a time window
summary: Count events per key in a sliding window, alert when the threshold is reached and suppress repeats for one window.
minutes: 35
difficulty: medium
tags: security, detection, windows
id: sec-detection/threshold-window
mode: stdout
---
Many attacks are visible only as a rate: five failed logins in a minute, twenty denied connections in ten seconds. A threshold rule says "N or more events for the same key within W seconds". Counting all events ever seen would alert on any busy account eventually; a window counts only the recent ones.

Define the window precisely, because off-by-one errors here decide whether a rule fires. When an event arrives at time `t`, the window holds events of the same key with timestamp strictly greater than `t - W`, up to and including `t`. Two events exactly `W` seconds apart are therefore not in the same window. Once a rule fires, it should not fire again on every following event; suppressing repeats for one window per key keeps analysts from drowning in duplicates (which links to [tuning](/learn/sec-detection/false-positive-tuning) later).

You met the idea of counting per source in [authentication log analysis](/learn/sec-forensics/auth-log-analysis); this time order and time matter.
