---
title: Review: a small detection pipeline
summary: Combine parsing, a sliding-window threshold, an allowlist and a follow-up correlation into one login-failure detector.
minutes: 35
difficulty: medium
tags: security, detection, review
id: sec-detection/review-detection-pipeline
mode: stdout
kind: review
---
This review joins four earlier pieces. From [threshold windows](/learn/sec-detection/threshold-window) comes the rule "N failures from one source within W seconds"; from [tuning](/learn/sec-detection/false-positive-tuning) comes the allowlist of sources that must never alert; from [authentication log analysis](/learn/sec-forensics/auth-log-analysis) comes the question that makes an alert useful: did the source later get in?

## The pipeline

Only `fail` events count toward the threshold, and allowlisted sources never alert. Each source alerts at most once per window, exactly as before: after an alert at `t`, that source is quiet until `t + W`. After the stream ends, answer the follow-up for every alert: the timestamp of the first `ok` event from the same source at or after the alert time, or `none`. An analyst reading `first-ok=none` and `first-ok=5120` triages them very differently, because the second one is a source that guessed its way in.

```text
W=60 N=3, allowlist: 10.0.0.*

1000 fail alice 192.0.2.8
1010 fail bob   192.0.2.8
1020 fail alice 192.0.2.8      count 3 -> ALERT
1030 fail alice 10.0.0.4       allowlisted, ignored
1050 ok   alice 192.0.2.8      first success after the alert
1060 fail alice 10.0.0.4
```

```text
ALERT 1020 192.0.2.8 failures=3 first-ok=1050
```

## What to watch for

- **Count only failures.** An `ok` from the same source in between does not reset or add to the count in this rule.
- **Allowlist first.** An allowlisted source is skipped entirely, so it can never produce an alert even with a huge failure count.
- **Counting is by source,** not by user: failures against different accounts from one address add up.
- **Follow-up looks forward.** The `ok` search runs over the whole stream after the alert, so you must keep all events until the end rather than answering while reading. An `ok` for a different source does not count.
- **Window bounds** are the familiar `(t-W, t]`, and suppression ends at `t + W` inclusive.

There is no new idea here, only care in combining familiar ones. When the output looks wrong, test each piece alone: print the counts without the allowlist, then add suppression, then the follow-up.
