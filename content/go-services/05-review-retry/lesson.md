---
title: "Review: Retry with a Budget"
summary: A consolidation exercise. Combine cancellable waits, per-attempt timeouts and error wrapping in one retry helper. No new ideas, just practice.
minutes: 30
difficulty: medium
kind: review
tags: context, errors, timeouts
id: go-services/review-retry
mode: harness
---
Time to combine what the first four lessons covered. You will write one helper, `Retry`, that calls a function again when it fails, with three rules:

1. Every attempt gets its **own timeout**, derived from the caller's context ([deadlines](/learn/go-services/deadlines)).
2. Between attempts it **waits**, but the wait must end the moment the caller's context does ([cancellation](/learn/go-services/cancellation)).
3. When it gives up it returns an error that still matches the last failure with `errors.Is` ([error wrapping](/learn/go-practice/error-wrapping)).

Nothing here is new. The only twist is deciding which context to check at each step: the caller's, or the per-attempt one.
