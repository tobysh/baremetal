Validate everything first (a map for duplicate IDs), then copy and sort the jobs by ID so the caller's slice is untouched and ties resolve by position.
===
Keep a small state per job: next attempt tick, attempt number and whether it is live. Each round, compute every live job's next event time as the cancel tick or attempt tick, pick the minimum, and stop when none is at or before the horizon.
===
Backoff<<(n-1) cannot overflow when Backoff <= 1<<20 and n <= 8, so plain uint64 arithmetic is safe after validation.
