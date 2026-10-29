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

Only `fail` events count toward the threshold and allowlisted sources never alert. Each source alerts at most once per window, exactly as before. After the stream ends, answer the follow-up for every alert: the timestamp of the first `ok` event from the same source at or after the alert time, or `none`. An analyst reading `first-ok=none` and `first-ok=5120` triages them very differently.

There is no new idea here, only care in combining familiar ones.
